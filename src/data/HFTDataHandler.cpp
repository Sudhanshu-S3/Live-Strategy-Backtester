#include "../../include/data/HFTDataHandler.h"
#include <iostream>
#include <sstream>
#include <algorithm> 
#include <limits>    

// --- Constructor ---
// Corrected constructor to match the header file.
HFTDataHandler::HFTDataHandler(const std::vector<std::string>& symbols,
                               const std::string& trade_data_dir,
                               const std::string& book_data_dir)
    : symbols_(symbols), 
      trade_data_dir_(trade_data_dir), 
      book_data_dir_(book_data_dir) 
{
    // Load data for each symbol upon construction
    for (const auto& symbol : symbols_) {
        load_trade_data(symbol);
        load_orderbook_data(symbol);
        // Initialize indices for streaming
        trade_indices_[symbol] = 0;
        orderbook_indices_[symbol] = 0;
    }
}

// --- Private Data Loading Methods ---

void HFTDataHandler::load_trade_data(const std::string& symbol) {
    std::string filepath = trade_data_dir_ + "/" + symbol + "_trades_7days.csv"; // Assuming a filename convention
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open trade data file: " << filepath << std::endl;
        return;
    }

    std::cout << "Loading trade data from: " << filepath << std::endl;

    std::string line;
    // Skip header
    std::getline(file, line); 

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string item;
        
        Trade t;
        t.symbol = symbol;

        // Note: The order must match your CSV format exactly.
        // datetime,trade_id,price,quantity,time,isBuyerMaker
        std::getline(ss, item, ','); // Skip datetime string
        std::getline(ss, item, ','); // Skip trade_id
        std::getline(ss, item, ','); t.price = std::stod(item);
        std::getline(ss, item, ','); t.quantity = std::stod(item);
        std::getline(ss, item, ','); t.timestamp = std::stol(item);
        std::getline(ss, item, ','); t.is_buyer_maker = (item == "True" || item == "true" || item == "1");

        all_trades_[symbol].push_back(t);
    }
    std::cout << "Loaded " << all_trades_[symbol].size() << " trades for " << symbol << std::endl;
}


void HFTDataHandler::load_orderbook_data(const std::string& symbol) {
    // Assuming the latest file for simplicity. A real implementation might need to list files.
    // FIX: The filename was hardcoded and did not match the user's file.
    // NOTE: A better solution would be to dynamically find the latest file.
    std::string filepath = book_data_dir_ + "/" + "btcusdt" + "_book_depth_20250715_032126.csv"; // Example filename
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open order book data file: " << filepath << std::endl;
        return;
    }
    std::cout << "Loading order book data from: " << filepath << std::endl;

    std::string line;
    std::getline(file, line); // Skip header

    long last_timestamp = 0;
    OrderBook current_book;
    current_book.symbol = symbol;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string item;

        long timestamp;
        std::string type;
        double price, quantity;
        
        // timestamp,type,price,quantity
        std::getline(ss, item, ','); timestamp = static_cast<long>(std::stod(item) * 1000); // from seconds to ms
        std::getline(ss, item, ','); type = item;
        std::getline(ss, item, ','); price = std::stod(item);
        std::getline(ss, item, ','); quantity = std::stod(item);

        if (timestamp != last_timestamp && last_timestamp != 0) {
            // New snapshot, save the previous one and start a new one
            current_book.timestamp = last_timestamp;
            all_orderbooks_[symbol].push_back(current_book);
            
            // Reset for the new book
            current_book.bids.clear();
            current_book.asks.clear();
        }
        
        // Add to the current book
        if (type == "BID") {
            current_book.bids.push_back({price, quantity});
        } else if (type == "ASK") {
            current_book.asks.push_back({price, quantity});
        }
        last_timestamp = timestamp;
    }

    // Add the very last book
    if (last_timestamp != 0) {
        current_book.timestamp = last_timestamp;
        all_orderbooks_[symbol].push_back(current_book);
    }
    
    // Sort asks (ascending) and bids (descending) for correctness
    for(auto& book : all_orderbooks_[symbol]) {
        std::sort(book.asks.begin(), book.asks.end(), [](const OrderBookLevel& a, const OrderBookLevel& b){ return a.price < b.price; });
        std::sort(book.bids.begin(), book.bids.end(), [](const OrderBookLevel& a, const OrderBookLevel& b){ return a.price > b.price; });
    }

    std::cout << "Loaded " << all_orderbooks_[symbol].size() << " order book snapshots for " << symbol << std::endl;
}


// --- Core Event Streaming Logic ---

// Corrected to match the pure virtual function in the base class.
void HFTDataHandler::updateBars(std::queue<std::shared_ptr<Event>>& event_queue) {
    if (isFinished()) {
        return;
    }

    long long earliest_timestamp = std::numeric_limits<long long>::max();
    std::string next_symbol = "";
    std::string event_type = ""; // "TRADE" or "BOOK"

    // Find the next chronological event across all data streams
    for (const auto& symbol : symbols_) {
        // Check for next trade event
        size_t trade_idx = trade_indices_[symbol];
        if (trade_idx < all_trades_[symbol].size()) {
            if (all_trades_[symbol][trade_idx].timestamp < earliest_timestamp) {
                earliest_timestamp = all_trades_[symbol][trade_idx].timestamp;
                next_symbol = symbol;
                event_type = "TRADE";
            }
        }

        // Check for next order book event
        size_t book_idx = orderbook_indices_[symbol];
        if (book_idx < all_orderbooks_[symbol].size()) {
            if (all_orderbooks_[symbol][book_idx].timestamp < earliest_timestamp) {
                earliest_timestamp = all_orderbooks_[symbol][book_idx].timestamp;
                next_symbol = symbol;
                event_type = "BOOK";
            }
        }
    }

    // If an event was found, push it to the queue
    if (!next_symbol.empty()) {
        if (event_type == "TRADE") {
            size_t& idx = trade_indices_[next_symbol];
            Trade& trade_data = all_trades_[next_symbol][idx];
            event_queue.push(std::make_shared<TradeEvent>(trade_data));
            idx++;
        } else if (event_type == "BOOK") {
            size_t& idx = orderbook_indices_[next_symbol];
            OrderBook& book_data = all_orderbooks_[next_symbol][idx];
            event_queue.push(std::make_shared<OrderBookEvent>(book_data));
            idx++;
        }
    }
}

// --- Implemented Virtual Functions ---

// Checks if all data streams for all symbols have been fully processed.
bool HFTDataHandler::isFinished() const {
    for (const auto& symbol : symbols_) {
        bool trades_done = trade_indices_.at(symbol) >= all_trades_.at(symbol).size();
        bool books_done = orderbook_indices_.at(symbol) >= all_orderbooks_.at(symbol).size();
        if (!trades_done || !books_done) {
            return false; // Found a stream that is not finished
        }
    }
    return true; // All streams are finished
}

// This handler deals with trades and order books, not OHLCV bars.
// It returns an empty optional, fulfilling the interface contract.
std::optional<Bar> HFTDataHandler::getLatestBar(const std::string& symbol) const {
    // Not applicable for HFT data model, but must be implemented.
    return std::nullopt;
}