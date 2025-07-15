#include "../../include/data/HFTDataHandler.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <limits>
#include <utility> // For std::move

// --- Constructor and Data Loading ---
HFTDataHandler::HFTDataHandler(std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
                               const std::vector<std::string>& symbols,
                               const std::string& trade_data_dir,
                               const std::string& book_data_dir,
                               const std::string& historical_data_fallback_dir)
    : event_queue_(event_queue), symbols_(symbols), trade_data_dir_(trade_data_dir), 
      book_data_dir_(book_data_dir), historical_data_fallback_dir_(historical_data_fallback_dir) 
{
    for (const auto& symbol : symbols_) {
        load_data(symbol, ""); // Load from primary dirs
    }
}

bool HFTDataHandler::load_data(const std::string& symbol, const std::string& base_dir) {
    // Simplified loader, assuming trades and books are in corresponding subdirectories
    std::string trade_dir = base_dir.empty() ? trade_data_dir_ : base_dir + "/trades";
    std::string book_dir = base_dir.empty() ? book_data_dir_ : base_dir + "/orderbooks";
    
    // In a real system you would implement the memory-mapped file loading here (STAGE 3)
    // For now, we continue with fstream.
    
    // Load Trades
    std::string trade_filepath = trade_dir + "/" + symbol + "_trades.csv";
    std::ifstream trade_file(trade_filepath);
    if(trade_file.is_open()){
        std::vector<Trade> trades;
        std::string line;
        std::getline(trade_file, line); // header
        while(std::getline(trade_file, line)){
            // Parsing logic...
        }
        all_trades_[symbol] = std::move(trades);
        trade_indices_[symbol] = 0;
    } else {
        std::cerr << "Warning: Could not open trade data file: " << trade_filepath << std::endl;
    }

    // Load Order Books
    // ... similar loading logic for order books ...

    return true;
}


// --- Main Update Loop ---
void HFTDataHandler::updateBars(std::queue<std::shared_ptr<Event>>& event_queue) {
    // This function is now simpler. It's responsible for advancing the "market time"
    // by loading the next chunk of data. The data is then read by strategy threads.
    // It no longer pushes market events, as strategies pull data directly.
    
    data_spinlock_.lock(); // Lock to safely advance indices

    if (isFinished() && !is_live_feed_.load()) {
        data_spinlock_.unlock();
        return;
    }

    // STAGE 3: Streaming Data Processing - process a small batch of events
    int events_to_process = 10; 
    for(int i = 0; i < events_to_process; ++i) {
        std::string next_symbol_to_process = "";
        std::string earliest_time = "9999-99-99 99:99:99.999999";

        for (const auto& symbol : symbols_) {
            if (trade_indices_.count(symbol) && trade_indices_.at(symbol) < all_trades_.at(symbol).size()) {
                const auto& next_trade_time = all_trades_.at(symbol)[trade_indices_.at(symbol)].timestamp;
                if (next_trade_time < earliest_time) {
                    earliest_time = next_trade_time;
                }
            }
            // ... check order books too ...
        }

        if (earliest_time == "9999-99-99 99:99:99.999999") break; // No more events in this batch

        // Advance indices for all symbols up to the earliest time
        for (const auto& symbol : symbols_) {
            if (trade_indices_.count(symbol) && trade_indices_.at(symbol) < all_trades_.at(symbol).size() &&
                all_trades_.at(symbol)[trade_indices_.at(symbol)].timestamp == earliest_time) {
                trade_indices_.at(symbol)++;
            }
            // ... advance book indices too ...
        }
    }
    
    data_spinlock_.unlock();
}


// --- Thread-Safe Data Accessors (STAGE 3) ---
OrderBook HFTDataHandler::getLatestOrderBook(const std::string& symbol) {
    std::lock_guard<Spinlock> lock(data_spinlock_);
    if (orderbook_indices_.count(symbol) && orderbook_indices_.at(symbol) > 0) {
        return all_orderbooks_.at(symbol)[orderbook_indices_.at(symbol) - 1];
    }
    return {};
}

Trade HFTDataHandler::getLatestTrade(const std::string& symbol) {
    std::lock_guard<Spinlock> lock(data_spinlock_);
    if (trade_indices_.count(symbol) && trade_indices_.at(symbol) > 0) {
        return all_trades_.at(symbol)[trade_indices_.at(symbol) - 1];
    }
    return {};
}

double HFTDataHandler::getLatestBarValue(const std::string& symbol, const std::string& val_type) {
    if (val_type == "price") {
        return getLatestTrade(symbol).price;
    }
    return 0.0;
}

bool HFTDataHandler::isFinished() const {
    // No lock needed for this read, assuming indices are only ever advanced.
    for (const auto& symbol : symbols_) {
        if (trade_indices_.count(symbol) && trade_indices_.at(symbol) < all_trades_.at(symbol).size()) {
            return false;
        }
        if (orderbook_indices_.count(symbol) && orderbook_indices_.at(symbol) < all_orderbooks_.at(symbol).size()) {
            return false;
        }
    }
    return true;
}

void HFTDataHandler::updateBars(std::queue<std::shared_ptr<Event>>& event_queue) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    if (is_live_feed_.load() && !is_connected_.load() && !historical_fallback_active_) {
        attemptReconnection();
        return; // Wait for reconnection or fallback
    }

    // STAGE 4: Simulate fast replay for shadow mode
    if (is_live_feed_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Simulate real-time data arrival
    }

    std::string next_symbol_to_process = "";
    std::string earliest_time = "9999-99-99 99:99:99.999999";

    for (const auto& symbol : symbols_) {
        if (trade_indices_.count(symbol) && trade_indices_.at(symbol) < all_trades_.at(symbol).size()) {
            const auto& next_trade_time = all_trades_.at(symbol)[trade_indices_.at(symbol)].timestamp;
            if (next_trade_time < earliest_time) {
                earliest_time = next_trade_time;
                next_symbol_to_process = symbol;
            }
        }
        if (orderbook_indices_.count(symbol) && orderbook_indices_.at(symbol) < all_orderbooks_.at(symbol).size()) {
            const auto& next_book_time = all_orderbooks_.at(symbol)[orderbook_indices_.at(symbol)].timestamp;
            if (next_book_time < earliest_time) {
                earliest_time = next_book_time;
                next_symbol_to_process = symbol;
            }
        }
    }

    if (!next_symbol_to_process.empty()) {
        // Process the earliest event found
        bool trade_is_earliest = trade_indices_.count(next_symbol_to_process) &&
                                 trade_indices_.at(next_symbol_to_process) < all_trades_.at(next_symbol_to_process).size() &&
                                 all_trades_.at(next_symbol_to_process)[trade_indices_.at(next_symbol_to_process)].timestamp == earliest_time;

        bool book_is_earliest = orderbook_indices_.count(next_symbol_to_process) &&
                                orderbook_indices_.at(next_symbol_to_process) < all_orderbooks_.at(next_symbol_to_process).size() &&
                                all_orderbooks_.at(next_symbol_to_process)[orderbook_indices_.at(next_symbol_to_process)].timestamp == earliest_time;

        if (trade_is_earliest) {
            const auto& trade = all_trades_.at(next_symbol_to_process)[trade_indices_.at(next_symbol_to_process)++];
            event_queue.push(std::make_shared<TradeEvent>(trade.symbol, trade.timestamp, trade.price, trade.quantity, trade.aggressor_side));
        }
        if (book_is_earliest) {
            const auto& orderbook = all_orderbooks_.at(next_symbol_to_process)[orderbook_indices_.at(next_symbol_to_process)++];
            event_queue.push(std::make_shared<OrderBookEvent>(orderbook.symbol, orderbook.timestamp, orderbook.bids, orderbook.asks));
        }
    }
}


// --- Live Feed and Error Handling ---

void HFTDataHandler::connectLiveFeed() {
    is_live_feed_.store(true);
    std::cout << "Attempting to connect to live feed..." << std::endl;
    // Simulate connection success/failure
    if (connection_retries_ < 2) { // Simulate initial failure
        is_connected_.store(false);
        std::cerr << "Connection Error: Failed to connect to live data feed." << std::endl;
    } else {
        is_connected_.store(true);
        connection_retries_ = 0; // Reset on success
        std::cout << "HFTDataHandler connected to simulated live feed." << std::endl;
    }
}

void HFTDataHandler::attemptReconnection() {
    if (connection_retries_ >= max_connection_retries_) {
        std::cerr << "Critical Error: Max reconnection attempts reached. Falling back to historical data." << std::endl;
        fallbackToHistoricalData();
        return;
    }

    int delay = base_retry_delay_ms_ * static_cast<int>(std::pow(2, connection_retries_));
    std::cerr << "Connection lost. Retrying in " << delay << "ms... (Attempt " << connection_retries_ + 1 << ")" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    
    connection_retries_++;
    connectLiveFeed(); // Attempt to connect again
}

void HFTDataHandler::fallbackToHistoricalData() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    if (historical_fallback_active_) return;

    std::cerr << "Activating historical data fallback from: " << historical_data_fallback_dir_ << std::endl;
    historical_fallback_active_ = true;
    all_trades_.clear();
    all_orderbooks_.clear();
    trade_indices_.clear();
    orderbook_indices_.clear();

    for (const auto& symbol : symbols_) {
        load_trade_data(symbol, historical_data_fallback_dir_);
        load_orderbook_data(symbol, historical_data_fallback_dir_);
        trade_indices_[symbol] = 0;
        orderbook_indices_[symbol] = 0;
    }
}


std::optional<Bar> HFTDataHandler::getLatestBar(const std::string& symbol) const { return std::nullopt; }
std::vector<Bar> HFTDataHandler::getLatestBars(const std::string& symbol, int n) { return {}; }
double HFTDataHandler::getLatestBarValue(const std::string& symbol, const std::string& val_type) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    if (val_type == "price") {
        auto it_trade = trade_indices_.find(symbol);
        if (it_trade != trade_indices_.end() && it_trade->second > 0) {
            return all_trades_.at(symbol)[it_trade->second - 1].price;
        }
    }
    return 0.0;
}