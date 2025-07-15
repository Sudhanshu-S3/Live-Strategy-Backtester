#include "data/DatabaseDataHandler.h"
#include "event/Event.h"
#include <iostream>
#include <nlohmann/json.hpp>

DatabaseDataHandler::DatabaseDataHandler(EventQueue& q, const std::string& connection_string,
                                         const std::string& symbol, const std::string& start_date,
                                         const std::string& end_date)
    : DataHandler(q), symbol(symbol), start_date(start_date), end_date(end_date) {
    
    try {
        conn = std::make_unique<pqxx::connection>(connection_string);
        if (conn->is_open()) {
            std::cout << "Database connection established." << std::endl;
        } else {
            throw std::runtime_error("Could not connect to database.");
        }
    } catch (const std::exception &e) {
        std::cerr << "Database connection failed: " << e.what() << std::endl;
        throw;
    }

    last_loaded_timestamp = start_date;
    load_chunk();
}

DatabaseDataHandler::~DatabaseDataHandler() {
    if (conn && conn->is_open()) {
        conn->disconnect();
        std::cout << "Database connection closed." << std::endl;
    }
}

void DatabaseDataHandler::load_chunk() {
    if (!continue_backtest) {
        return;
    }

    std::cout << "Loading next data chunk from " << last_loaded_timestamp << "..." << std::endl;
    
    try {
        pqxx::work txn(*conn);
        // We query for order_books here, but you can UNION with trades if needed
        std::string query = 
            "SELECT time, bids, asks FROM order_books "
            "WHERE symbol = " + txn.quote(symbol) + " "
            "AND time > " + txn.quote(last_loaded_timestamp) + " "
            "AND time <= " + txn.quote(end_date) + " "
            "ORDER BY time ASC LIMIT " + std::to_string(CHUNK_SIZE) + ";";

        data_chunk = txn.exec(query);
        data_iterator = data_chunk.begin();
        
        if (data_iterator == data_chunk.end()) {
            std::cout << "No more data to load. Ending backtest." << std::endl;
            continue_backtest = false;
        } else {
            // Get the timestamp of the last row in the chunk to use as the starting point for the next query
            last_loaded_timestamp = data_chunk.back()[0].as<std::string>();
        }
    } catch (const std::exception &e) {
        std::cerr << "Error loading data chunk: " << e.what() << std::endl;
        continue_backtest = false;
    }
}

void DatabaseDataHandler::get_next_event() {
    if (!continue_backtest) {
        return;
    }

    if (data_iterator == data_chunk.end()) {
        load_chunk();
        if (!continue_backtest) {
            return;
        }
    }

    // Process the current row
    auto row = *data_iterator;
    std::string timestamp_str = row[0].as<std::string>();
    std::string bids_json_str = row[1].as<std::string>();
    std::string asks_json_str = row[2].as<std::string>();

    try {
        // Parse the timestamp and JSON strings
        auto bids_json = nlohmann::json::parse(bids_json_str);
        auto asks_json = nlohmann::json::parse(asks_json_str);

        OrderBook book(timestamp_str, symbol);
        for (const auto& bid : bids_json) {
            book.bids.emplace_back(std::stod(bid[0].get<std::string>()), std::stod(bid[1].get<std::string>()));
        }
        for (const auto& ask : asks_json) {
            book.asks.emplace_back(std::stod(ask[0].get<std::string>()), std::stod(ask[1].get<std::string>()));
        }

        // Push the event to the queue
        events_queue.push(std::make_unique<OrderBookEvent>(std::move(book)));

    } catch (const std::exception& e) {
        std::cerr << "Error parsing order book data at " << timestamp_str << ": " << e.what() << std::endl;
    }
    
    // Move to the next row
    ++data_iterator;
}


void DatabaseDataHandler::load_chunk() {
    if (!continue_backtest) return;

    std::cout << "Loading next data chunk from " << last_loaded_timestamp << "..." << std::endl;
    
    try {
        pqxx::work txn(*conn);
        // This is the key change: a UNION ALL query to merge data sources
        std::string query = 
            "SELECT time, 'order_book' as event_type, symbol, bids, asks, NULL as headline, NULL as sentiment FROM order_books "
            "WHERE symbol = " + txn.quote(symbol) + " AND time > " + txn.quote(last_loaded_timestamp) + " AND time <= " + txn.quote(end_date) + " "
            "UNION ALL "
            "SELECT time, 'news' as event_type, symbol, NULL as bids, NULL as asks, headline, sentiment_score as sentiment FROM news_articles "
            "WHERE symbol = " + txn.quote(symbol) + " AND time > " + txn.quote(last_loaded_timestamp) + " AND time <= " + txn.quote(end_date) + " "
            "ORDER BY time ASC LIMIT " + std::to_string(CHUNK_SIZE) + ";";

        data_chunk = txn.exec(query);
        data_iterator = data_chunk.begin();
        
        if (data_iterator == data_chunk.end()) {
            std::cout << "No more data to load. Ending backtest." << std::endl;
            continue_backtest = false;
        } else {
            last_loaded_timestamp = data_chunk.back()["time"].as<std::string>();
        }
    } catch (const std::exception &e) {
        std::cerr << "Error loading data chunk: " << e.what() << std::endl;
        continue_backtest = false;
    }
}

void DatabaseDataHandler::get_next_event() {
    if (!continue_backtest) return;

    if (data_iterator == data_chunk.end()) {
        load_chunk();
        if (!continue_backtest) return;
    }

    auto row = *data_iterator;
    std::string event_type = row["event_type"].as<std::string>();
    std::string timestamp_str = row["time"].as<std::string>();
    std::string event_symbol = row["symbol"].as<std::string>();

    try {
        if (event_type == "order_book") {
            std::string bids_json_str = row["bids"].as<std::string>();
            std::string asks_json_str = row["asks"].as<std::string>();
            
            auto bids_json = nlohmann::json::parse(bids_json_str);
            auto asks_json = nlohmann::json::parse(asks_json_str);

            OrderBook book(timestamp_str, event_symbol);
            for (const auto& bid : bids_json) {
                book.bids.emplace_back(std::stod(bid[0].get<std::string>()), std::stod(bid[1].get<std::string>()));
            }
            for (const auto& ask : asks_json) {
                book.asks.emplace_back(std::stod(ask[0].get<std::string>()), std::stod(ask[1].get<std::string>()));
            }
            events_queue.push(std::make_unique<OrderBookEvent>(std::move(book)));

        } else if (event_type == "news") {
            std::string headline = row["headline"].as<std::string>();
            double sentiment = row["sentiment"].as<double>();

            events_queue.push(std::make_unique<NewsEvent>(event_symbol, timestamp_str, headline, sentiment));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing event data at " << timestamp_str << ": " << e.what() << std::endl;
    }
    
    ++data_iterator;
}
