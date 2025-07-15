#include "../../include/data/DatabaseDataHandler.h"
#include "../../include/event/Event.h"
#include <iostream>
#include <stdexcept>

DatabaseDataHandler::DatabaseDataHandler(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue, 
                                         const std::string& connection_string,
                                         const std::vector<std::string>& symbols, 
                                         const std::string& start_date,
                                         const std::string& end_date)
    : event_queue_(event_queue), 
      symbols_(symbols), 
      start_date_(start_date), 
      end_date_(end_date),
      last_loaded_timestamp_(start_date) {
    
    try {
        conn = std::make_unique<pqxx::connection>(connection_string);
        if (!conn->is_open()) {
            throw std::runtime_error("Could not connect to database.");
        }
        std::cout << "Database connection established." << std::endl;
        // Load initial chunk for all symbols
        for(const auto& symbol : symbols_) {
            data_iterators_[symbol] = data_chunks_[symbol].end(); // Initialize iterator
            load_chunk(symbol);
        }
    } catch (const std::exception &e) {
        std::cerr << "Database connection failed: " << e.what() << std::endl;
        throw;
    }
}

DatabaseDataHandler::~DatabaseDataHandler() {
    if (conn && conn->is_open()) {
        conn->disconnect();
        std::cout << "Database connection closed." << std::endl;
    }
}

void DatabaseDataHandler::load_chunk(const std::string& symbol) {
    std::cout << "Loading next data chunk for " << symbol << " from " << last_loaded_timestamp_ << "..." << std::endl;
    
    try {
        pqxx::work txn(*conn);
        std::string query = 
            "SELECT time, open, high, low, close, volume FROM bars "
            "WHERE symbol = " + txn.quote(symbol) + " "
            "AND time > " + txn.quote(last_loaded_timestamp_) + " "
            "AND time <= " + txn.quote(end_date_) + " "
            "ORDER BY time ASC LIMIT " + std::to_string(CHUNK_SIZE) + ";";

        data_chunks_[symbol] = txn.exec(query);
        data_iterators_[symbol] = data_chunks_[symbol].begin();
        
        if (data_iterators_[symbol] != data_chunks_[symbol].end()) {
            // This needs to be smarter for multiple symbols. For now, we just update it.
            last_loaded_timestamp_ = data_chunks_[symbol].back()[0].as<std::string>();
        }
    } catch (const std::exception &e) {
        std::cerr << "Error loading data chunk for " << symbol << ": " << e.what() << std::endl;
    }
}

bool DatabaseDataHandler::isFinished() const {
    for(const auto& symbol : symbols_) {
        if (data_iterators_.at(symbol) != data_chunks_.at(symbol).end()) {
            return false; // Found a chunk with data
        }
    }
    // Could also check if a new chunk could be loaded, but for now this is fine.
    return true;
}

void DatabaseDataHandler::updateBars() {
    if (isFinished()) {
        return;
    }

    // Find next event across all symbol chunks
    std::string next_symbol_to_process = "";
    pqxx::result::const_iterator next_iterator;
    std::string earliest_time = "9999-99-99";

    for (const auto& symbol : symbols_) {
        auto& it = data_iterators_.at(symbol);
        if (it == data_chunks_.at(symbol).end()) {
            load_chunk(symbol); // Attempt to load more data
            it = data_iterators_.at(symbol); // Reset iterator
        }

        if (it != data_chunks_.at(symbol).end()) {
            std::string current_time = (*it)[0].as<std::string>();
            if (current_time < earliest_time) {
                earliest_time = current_time;
                next_symbol_to_process = symbol;
            }
        }
    }

    if (!next_symbol_to_process.empty()) {
        auto& row = *data_iterators_.at(next_symbol_to_process);
        
        Bar bar;
        bar.symbol = next_symbol_to_process;
        bar.timestamp = row[0].as<std::string>();
        bar.open = row[1].as<double>();
        bar.high = row[2].as<double>();
        bar.low = row[3].as<double>();
        bar.close = row[4].as<double>();
        bar.volume = row[5].as<long long>();

        event_queue_->push(std::make_shared<MarketEvent>(bar.symbol, bar.timestamp));
        
        data_iterators_.at(next_symbol_to_process)++;
    }
}

// Placeholder implementations for other interface methods
std::optional<Bar> DatabaseDataHandler::getLatestBar(const std::string& symbol) const { return std::nullopt; }
double DatabaseDataHandler::getLatestBarValue(const std::string& symbol, const std::string& val_type) { return 0.0; }
std::vector<Bar> DatabaseDataHandler::getLatestBars(const std::string& symbol, int n) { return {}; }

