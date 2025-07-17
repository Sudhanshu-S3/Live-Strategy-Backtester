// --- Paste this entire block into src/data/WebSocketDataHandler.cpp ---

#include "../../include/data/WebSocketDataHandler.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <iomanip>

// Helper function to report errors
void fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

WebSocketDataHandler::WebSocketDataHandler(
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
    const std::vector<std::string>& symbols,
    const std::string& host,
    const std::string& port,
    const std::string& target)
    : event_queue_(std::move(event_queue)),
      symbols_(symbols),
      host_(host),
      port_(port),
      target_(target),
      resolver_(ioc_),
      ws_(ioc_, ctx_)
{
    // Initialize SSL context
    ctx_.set_default_verify_paths();
    ctx_.set_verify_mode(ssl::verify_peer);
    
    // Initialize data structures for each symbol
    for (const auto& symbol : symbols_) {
        latest_bars_map_[symbol] = Bar{};
        trade_counts_[symbol] = 0;
    }
    
    std::cout << "WebSocketDataHandler initialized for symbols: ";
    for (const auto& symbol : symbols_) {
        std::cout << symbol << " ";
    }
    std::cout << std::endl;
}

WebSocketDataHandler::~WebSocketDataHandler() {
    stop();
}

void WebSocketDataHandler::connect() {
    std::cout << "Connecting to WebSocket at " << host_ << ":" << port_ << target_ << std::endl;
    finished_ = false;

    // Look up the domain name
    resolver_.async_resolve(
        host_,
        port_,
        beast::bind_front_handler(
            &WebSocketDataHandler::on_resolve,
            shared_from_this()));

    // Start the I/O context in its own thread
    ioc_thread_ = std::thread([this](){ 
        try {
            ioc_.run(); 
            std::cout << "WebSocket I/O context finished running." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "WebSocket I/O context exception: " << e.what() << std::endl;
        }
    });
}

void WebSocketDataHandler::stop() {
    std::cout << "Stopping WebSocket connection..." << std::endl;
    
    // Cancel any outstanding operations
    beast::error_code ec;
    ws_.close(websocket::close_code::normal, ec);
    
    // Stop the I/O context
    ioc_.stop();
    
    // Wait for the thread to complete
    if (ioc_thread_.joinable()) {
        ioc_thread_.join();
    }
    
    finished_ = true;
    std::cout << "WebSocket connection stopped." << std::endl;
}

void WebSocketDataHandler::on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
    if (ec) {
        fail(ec, "resolve");
        return;
    }

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if(!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str())) {
        ec = beast::error_code(static_cast<int>(::ERR_get_error()),
            net::error::get_ssl_category());
        fail(ec, "set SNI Hostname");
        return;
    }

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(ws_).async_connect(
        results,
        beast::bind_front_handler(
            &WebSocketDataHandler::on_connect,
            shared_from_this()));
}

void WebSocketDataHandler::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
    if (ec) {
        fail(ec, "connect");
        return;
    }

    // Perform the SSL handshake
    ws_.next_layer().async_handshake(
        ssl::stream_base::client,
        beast::bind_front_handler(
            &WebSocketDataHandler::on_ssl_handshake,
            shared_from_this()));
}

void WebSocketDataHandler::on_ssl_handshake(beast::error_code ec) {
    if (ec) {
        fail(ec, "ssl_handshake");
        return;
    }

    beast::get_lowest_layer(ws_).expires_never();

    ws_.set_option(
        websocket::stream_base::timeout::suggested(
            beast::role_type::client));

    ws_.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req) {
            req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-cpp");
        }));

    // Set suggested timeout settings for the websocket
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

    // Set a decorator to change the User-Agent of the handshake
    ws_.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req) {
            req.set(http::field::user_agent, 
                std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async");
        }));

    // Perform the websocket handshake
    ws_.async_handshake(host_, target_,
        beast::bind_front_handler(
            &WebSocketDataHandler::on_handshake,
            shared_from_this()));
}

void WebSocketDataHandler::on_handshake(beast::error_code ec) {
    if (ec) {
        fail(ec, "handshake");
        return;
    }

    std::cout << "WebSocket handshake successful. Connected to " << host_ << target_ << std::endl;

    // Start reading
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &WebSocketDataHandler::on_read,
            shared_from_this()));
}

void WebSocketDataHandler::on_write(beast::error_code ec, std::size_t) {
    if (ec) {
        fail(ec, "write");
        return;
    }

    // Read a message into our buffer
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &WebSocketDataHandler::on_read,
            shared_from_this()));
}

void WebSocketDataHandler::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
        fail(ec, "read");
        return;
    }

    // Process the message
    std::string message = beast::buffers_to_string(buffer_.data());
    process_message(message);
    
    // Clear the buffer
    buffer_.consume(bytes_transferred);

    // Queue up another read
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &WebSocketDataHandler::on_read,
            shared_from_this()));
}

void WebSocketDataHandler::on_close(beast::error_code ec) {
    if (ec) {
        fail(ec, "close");
    }

    // If we get here then the connection is closed gracefully
    std::cout << "WebSocket connection closed gracefully" << std::endl;
}

void WebSocketDataHandler::process_message(const std::string& message) {
    try {
        auto json_msg = nlohmann::json::parse(message);

        if (json_msg.contains("e") && json_msg["e"] == "trade") {
            // ... existing trade processing logic ...
            std::string symbol = json_msg["s"];
            long long timestamp = json_msg["T"];
            double price = std::stod(json_msg["p"].get<std::string>());
            double quantity = std::stod(json_msg["q"].get<std::string>());
            std::string side = json_msg["m"] ? "SELL" : "BUY";

            auto trade = std::make_shared<TradeEvent>(symbol, timestamp, price, quantity, side);
            // Fix: Just push trade directly - it's already a shared_ptr<Event> after casting
            event_queue_->push(std::make_shared<std::shared_ptr<Event>>(trade));


            // Update the latest bar
            Bar& bar = latest_bars_map_.at(symbol);
            // Fix: bar.timestamp is a string but you're comparing with int 0
            if (bar.timestamp.empty()) { // First trade for this symbol
                bar.timestamp = std::to_string(timestamp);
                bar.symbol = symbol;
                bar.open = bar.high = bar.low = bar.close = price;
                bar.volume = quantity;
            } else {
                // Update existing bar
                bar.high = std::max(bar.high, price);
                bar.low = std::min(bar.low, price);
                bar.close = price;
                bar.volume += quantity;
            }
            
            // Notify any listeners
            if (on_new_data_) {
                on_new_data_();
            }
        } else if (json_msg.contains("e") && json_msg["e"] == "depthUpdate") {
            // Process order book updates
            std::string symbol = json_msg["s"];
            long long timestamp = json_msg["E"];
            
            // Create order book event
            auto orderbook = std::make_shared<OrderBookEvent>(symbol, timestamp);
            
            // Also update our stored order book
            OrderBook& stored_book = latest_orderbooks_[symbol];
            stored_book.timestamp = timestamp;
            stored_book.symbol = symbol;
            
            // Add bid levels
            if (json_msg.contains("b") && json_msg["b"].is_array()) {
                stored_book.bids.clear(); // Clear old levels before update
                for (const auto& bid : json_msg["b"]) {
                    if (bid.is_array() && bid.size() >= 2) {
                        double price = std::stod(bid[0].get<std::string>());
                        double quantity = std::stod(bid[1].get<std::string>());
                        orderbook->addBidLevel(price, quantity);
                        // Option 1: If you only want to update quantity but keep other pair data
                        stored_book.bids[price].second = quantity;
                        // Option 2: If you want to replace the entire pair
                        //stored_book.bids[price] = std::make_pair(price, quantity);
                    }
                }
            }
            
            // Add ask levels
            if (json_msg.contains("a") && json_msg["a"].is_array()) {
                stored_book.asks.clear(); // Clear old levels before update
                for (const auto& ask : json_msg["a"]) {
                    if (ask.is_array() && ask.size() >= 2) {
                        double price = std::stod(ask[0].get<std::string>());
                        double quantity = std::stod(ask[1].get<std::string>());
                        orderbook->addAskLevel(price, quantity);
                        // Option 1: If you only want to update quantity but keep other pair data
                        stored_book.asks[price].second = quantity;
                        // Option 2: If you want to replace the entire pair
                        //stored_book.asks[price] = std::make_pair(price, quantity);
                    }
                }
            }
            
            // Print order book update summary
            std::cout << "ORDER BOOK: " << symbol << " | " 
                      << "Bids: " << orderbook->getBidLevels().size() << " | "
                      << "Asks: " << orderbook->getAskLevels().size() << std::endl;
            
            // Push order book event
            event_queue_->push(std::make_shared<std::shared_ptr<Event>>(orderbook));

            
            // Notify any listeners
            if (on_new_data_) {
                on_new_data_();
            }
        }
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON parse error in WebSocketDataHandler: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in WebSocketDataHandler: " << e.what() << std::endl;
    }
}

void WebSocketDataHandler::setOnNewDataCallback(std::function<void()> callback) {
    on_new_data_ = std::move(callback);
}

// --- DataHandler Interface Implementation ---

void WebSocketDataHandler::updateBars() {
    // This is handled asynchronously by the on_read loop
}

bool WebSocketDataHandler::isFinished() const {
    return finished_;
}

std::optional<Bar> WebSocketDataHandler::getLatestBar(const std::string& symbol) const {
    auto it = latest_bars_map_.find(symbol);
    if (it != latest_bars_map_.end() && std::stoll(it->second.timestamp) > 0) {
        return it->second;
    }
    return std::nullopt;
}

double WebSocketDataHandler::getLatestBarValue(const std::string& symbol, const std::string& val_type) {
    auto bar_opt = getLatestBar(symbol);
    if (!bar_opt) {
        return 0.0;
    }
    
    const Bar& bar = *bar_opt;
    
    if (val_type == "open") return bar.open;
    if (val_type == "high") return bar.high;
    if (val_type == "low") return bar.low;
    if (val_type == "close") return bar.close;
    if (val_type == "volume") return bar.volume;
    
    return 0.0;
}

std::vector<Bar> WebSocketDataHandler::getLatestBars(const std::string& symbol, int n) {
    // In a real implementation, we would keep a history of bars
    // For now, just return the latest bar if available
    std::vector<Bar> bars;
    auto bar_opt = getLatestBar(symbol);
    if (bar_opt) {
        bars.push_back(*bar_opt);
    }
    return bars;
}

std::optional<OrderBook> WebSocketDataHandler::getLatestOrderBook(const std::string& symbol) const {
    auto it = latest_orderbooks_.find(symbol);
    if (it != latest_orderbooks_.end()) {
        return it->second;
    }
    return std::nullopt;
}

const std::vector<std::string>& WebSocketDataHandler::getSymbols() const {
    // Return the symbols we're tracking
    return symbols_;
}

void WebSocketDataHandler::notifyOnNewData(std::function<void()> callback) {
    // Store the callback for later use when new data arrives
    on_new_data_ = std::move(callback);
}