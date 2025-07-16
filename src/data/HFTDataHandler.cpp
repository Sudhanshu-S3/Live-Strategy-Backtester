#include "../../include/data/HFTDataHandler.h"
#include "../../include/event/Event.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <limits>
#include <utility>
#include <chrono>
#include <mutex>
#include <fstream>
#include <thread>
#include <cmath> // Required for std::pow

// Helper to convert string timestamp to a long long
long long timestampToLong(const std::string& timestamp) {
    // This is a simplified conversion. A robust implementation would handle timezones
    // and use a more reliable parsing method.
    std::string clean_ts = timestamp;
    clean_ts.erase(std::remove(clean_ts.begin(), clean_ts.end(), '-'), clean_ts.end());
    clean_ts.erase(std::remove(clean_ts.begin(), clean_ts.end(), ':'), clean_ts.end());
    clean_ts.erase(std::remove(clean_ts.begin(), clean_ts.end(), ' '), clean_ts.end());
    clean_ts.erase(std::remove(clean_ts.begin(), clean_ts.end(), '.'), clean_ts.end());
    try {
        return std::stoll(clean_ts);
    } catch (const std::invalid_argument& e) {
        return 0;
    }
}


HFTDataHandler::HFTDataHandler(
    // FIX: Constructor now takes a queue of std::shared_ptr<Event>
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
    const std::vector<std::string>& symbols,
    const std::string& trade_data_dir,
    const std::string& book_data_dir,
    const std::string& historical_data_fallback_dir,
    const std::string& start_date,
    const std::string& end_date
) : event_queue_(event_queue), symbols_(symbols), trade_data_dir_(trade_data_dir), 
      book_data_dir_(book_data_dir), historical_data_fallback_dir_(historical_data_fallback_dir) 
{
    for (const auto& symbol : symbols_) {
        // Initially load historical data. The live feed can take over later.
        if (!historical_data_fallback_dir_.empty()) {
            load_data(symbol, historical_data_fallback_dir_, start_date, end_date);
        }
    }
}

void HFTDataHandler::updateBars() {
    std::shared_ptr<Event> event_to_push;
    {
        std::lock_guard<Spinlock> lock(data_spinlock_);
        
        long long earliest_trade_time = std::numeric_limits<long long>::max();
        std::string trade_symbol = "";
        for (const auto& symbol : symbols_) {
            if (trade_indices_.count(symbol) && trade_indices_.at(symbol) < all_trades_.at(symbol).size()) {
                if (all_trades_.at(symbol)[trade_indices_.at(symbol)].timestamp < earliest_trade_time) {
                    earliest_trade_time = all_trades_.at(symbol)[trade_indices_.at(symbol)].timestamp;
                    trade_symbol = symbol;
                }
            }
        }

        long long earliest_book_time = std::numeric_limits<long long>::max();
        std::string book_symbol = "";
        for (const auto& symbol : symbols_) {
            if (orderbook_indices_.count(symbol) && orderbook_indices_.at(symbol) < all_orderbooks_.at(symbol).size()) {
                if (all_orderbooks_.at(symbol)[orderbook_indices_.at(symbol)].timestamp < earliest_book_time) {
                    earliest_book_time = all_orderbooks_.at(symbol)[orderbook_indices_.at(symbol)].timestamp;
                    book_symbol = symbol;
                }
            }
        }

        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();

        if (!trade_symbol.empty() && (trade_symbol == book_symbol || earliest_trade_time <= earliest_book_time)) {
            const auto& trade = all_trades_.at(trade_symbol)[trade_indices_.at(trade_symbol)++];
            auto event = std::make_shared<TradeEvent>(trade.symbol, trade.timestamp, trade.price, trade.quantity, trade.aggressor_side);
            event->timestamp_received = now;
            event_to_push = event;
        } else if (!book_symbol.empty()) {
            const auto& book = all_orderbooks_.at(book_symbol)[orderbook_indices_.at(book_symbol)++];
            latest_orderbooks_[book.symbol] = book; // Store the latest book
            auto event = std::make_shared<OrderBookEvent>(book);
            event->timestamp_received = now;
            event_to_push = event;
        }
    }
    if (event_to_push) {
        // FIX: Push the shared_ptr directly. This is safe and manages memory correctly.
        // The original code (using .get() and .reset()) was buggy and would cause a crash.
        event_queue_->push(std::make_shared<std::shared_ptr<Event>>(event_to_push));
    }
    notifyNewData();
}

bool HFTDataHandler::isFinished() const {
    std::lock_guard<Spinlock> lock(data_spinlock_);
    for (const auto& symbol : symbols_) {
        if (trade_indices_.count(symbol) && trade_indices_.at(symbol) < all_trades_.at(symbol).size()) {
            return false;
        }
        if (orderbook_indices_.count(symbol) && orderbook_indices_.at(symbol) < all_orderbooks_.at(symbol).size()) {
            return false;
        }
    }
    return is_live_feed_ ? false : true;
}

std::optional<Bar> HFTDataHandler::getLatestBar(const std::string& symbol) const {
    // Bar construction logic from trades/order books would be needed here.
    // This is a placeholder implementation.
    return std::nullopt;
}

double HFTDataHandler::getLatestBarValue(const std::string& symbol, const std::string& val_type) {
    // Bar construction logic would be needed here.
    // This is a placeholder implementation.
    return 0.0;
}

std::vector<Bar> HFTDataHandler::getLatestBars(const std::string& symbol, int n) {
    // Bar construction logic would be needed here.
    // This is a placeholder implementation.
    return {};
}

void HFTDataHandler::connectLiveFeed() {
    is_live_feed_ = true;
    is_connected_ = true;
    connection_retries_ = 0;
    std::cout << "Live feed connected." << std::endl;
    // FIX: Use std::make_shared to create the event and push the resulting shared_ptr.
    (*event_queue_).push(std::make_shared<std::shared_ptr<Event>>(std::make_shared<DataSourceStatusEvent>(DataSourceStatus::CONNECTED, "Live feed connected.")));
}

void HFTDataHandler::attemptReconnection() {
    if (historical_fallback_active_) return;

    // FIX: Use std::make_shared for safety.
    (*event_queue_).push(std::make_shared<std::shared_ptr<Event>>(std::make_shared<DataSourceStatusEvent>(DataSourceStatus::DISCONNECTED, "Live feed connection lost.")));
    is_connected_ = false;

    while (connection_retries_ < max_connection_retries_ && !is_connected_) {
        connection_retries_++;
        // FIX: Use std::make_shared for safety.
        (*event_queue_).push(std::make_shared<std::shared_ptr<Event>>(
            std::make_shared<DataSourceStatusEvent>(DataSourceStatus::RECONNECTING,
            "Attempting to reconnect (" + std::to_string(connection_retries_) + "/" + std::to_string(max_connection_retries_) + ")...")));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(base_retry_delay_ms_ * (long long)std::pow(2, connection_retries_ - 1)));
        
        // Simulate reconnection attempt
        bool success = (rand() % 2 == 0); // 50% chance of success
        if (success) {
            is_connected_ = true;
            // FIX: Use std::make_shared for safety.
            (*event_queue_).push(std::make_shared<std::shared_ptr<Event>>(std::make_shared<DataSourceStatusEvent>(DataSourceStatus::CONNECTED, "Reconnection successful.")));
            std::cout << "Reconnection successful." << std::endl;
            connection_retries_ = 0;
        }
    }

    if (!is_connected_) {
        std::cerr << "Reconnection failed after " << max_connection_retries_ << " attempts. Falling back to historical data." << std::endl;
        fallbackToHistoricalData();
    }
}

void HFTDataHandler::fallbackToHistoricalData() {
    if (!historical_data_fallback_dir_.empty()) {
        historical_fallback_active_ = true;
        is_live_feed_ = false;
        std::cout << "Switching to historical data feed." << std::endl;
        // FIX: This line was already correct, but now it will compile with the updated queue type.
        (*event_queue_).push(std::make_shared<std::shared_ptr<Event>>(std::make_shared<DataSourceStatusEvent>(DataSourceStatus::FALLBACK_ACTIVE, "Fell back to historical data.")));
    } else {
        std::cerr << "No historical data fallback directory specified. System will halt." << std::endl;
        // Or push a critical error event
    }
}

bool HFTDataHandler::load_data(const std::string& symbol, const std::string& dir, const std::string& start_date, const std::string& end_date) {
    std::string filepath = dir + "/" + symbol + "-trades.csv"; // Assuming a naming convention
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open historical data file for " << symbol << " at " << filepath << std::endl;
        return false;
    }

    std::string line;
    getline(file, line); // Skip header

    while (getline(file, line)) {
        std::stringstream ss(line);
        std::string item;
        Trade trade;
        trade.symbol = symbol;
        int i = 0;
        while(std::getline(ss, item, ',')) {
            if (i == 0) trade.timestamp = std::stoll(item);
            else if (i == 1) trade.price = std::stod(item);
            else if (i == 2) trade.quantity = std::stod(item);
            else if (i == 3) trade.aggressor_side = item;
            i++;
        }
        all_trades_[symbol].push_back(trade);
    }
    trade_indices_[symbol] = 0;
    return true;
}


std::optional<OrderBook> HFTDataHandler::getLatestOrderBook(const std::string& symbol) const {
    std::lock_guard<Spinlock> lock(data_spinlock_);
    auto it = latest_orderbooks_.find(symbol);
    if (it != latest_orderbooks_.end()) {
        return it->second;
    }
    return std::nullopt;
}