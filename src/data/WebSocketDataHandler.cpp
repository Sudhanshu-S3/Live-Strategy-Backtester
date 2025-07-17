// --- Paste this entire block into src/data/WebSocketDataHandler.cpp ---

#include "../../include/data/WebSocketDataHandler.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>

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
      target_(target)
{
}

WebSocketDataHandler::~WebSocketDataHandler() {
    stop();
}

void WebSocketDataHandler::connect() {
    finished_ = false;

    // Look up the domain name
    resolver_.async_resolve(
        host_,
        port_,
        beast::bind_front_handler(
            &WebSocketDataHandler::on_resolve,
            shared_from_this()));

    // Start the I/O context in its own thread
    ioc_thread_ = std::thread([this](){ ioc_.run(); });
}

void WebSocketDataHandler::stop() {
    if (finished_.exchange(true)) {
        return; // Already stopping or stopped
    }
    
    // Post a message to the io_context to close the socket
    if (ioc_.get_executor().running_in_this_thread()) {
         ws_.async_close(websocket::close_code::normal, beast::bind_front_handler(&WebSocketDataHandler::on_close, shared_from_this()));
    } else {
        net::post(ws_.get_executor(), [self = shared_from_this()]() {
            self->ws_.async_close(websocket::close_code::normal, beast::bind_front_handler(&WebSocketDataHandler::on_close, self));
        });
    }

    // Join the thread
    if (ioc_thread_.joinable()) {
        ioc_thread_.join();
    }
}

void WebSocketDataHandler::on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
    if (ec) return fail(ec, "resolve");

    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

    beast::get_lowest_layer(ws_).async_connect(
        results,
        beast::bind_front_handler(
            &WebSocketDataHandler::on_connect,
            shared_from_this()));
}

void WebSocketDataHandler::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
    if (ec) return fail(ec, "connect");

    ws_.next_layer().async_handshake(
        ssl::stream_base::client,
        beast::bind_front_handler(
            &WebSocketDataHandler::on_ssl_handshake,
            shared_from_this()));
}

void WebSocketDataHandler::on_ssl_handshake(beast::error_code ec) {
    if (ec) return fail(ec, "ssl_handshake");

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

    ws_.async_handshake(host_, target_,
        beast::bind_front_handler(
            &WebSocketDataHandler::on_handshake,
            shared_from_this()));
}

void WebSocketDataHandler::on_handshake(beast::error_code ec) {
    if (ec) return fail(ec, "handshake");
    
    std::cout << "WebSocket Connection Established." << std::endl;
    
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &WebSocketDataHandler::on_read,
            shared_from_this()));
}

void WebSocketDataHandler::on_write(beast::error_code ec, std::size_t) {
    if (ec) return fail(ec, "write");

    buffer_.consume(buffer_.size());

    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &WebSocketDataHandler::on_read,
            shared_from_this()));
}

void WebSocketDataHandler::on_read(beast::error_code ec, std::size_t) {
    if (ec == websocket::error::closed) {
        std::cout << "WebSocket connection closed." << std::endl;
        finished_ = true;
        return;
    }
    
    if (ec) return fail(ec, "read");
    
    process_message(beast::buffers_to_string(buffer_.data()));

    buffer_.consume(buffer_.size());
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &WebSocketDataHandler::on_read,
            shared_from_this()));
}

void WebSocketDataHandler::on_close(beast::error_code ec) {
    if (ec) {
        // This can happen if the remote side closes the connection abruptly.
        // It's not necessarily a critical error to fail on.
        // fail(ec, "close"); 
    }
    std::cout << "WebSocket connection shut down." << std::endl;
}

void WebSocketDataHandler::process_message(const std::string& message) {
    try {
        auto json_msg = nlohmann::json::parse(message);

        if (json_msg.contains("e") && json_msg["e"] == "trade") {
            std::string symbol = json_msg["s"];
            long long timestamp = json_msg["T"];
            double price = std::stod(json_msg["p"].get<std::string>());
            double quantity = std::stod(json_msg["q"].get<std::string>());
            std::string side = json_msg["m"] ? "SELL" : "BUY";

            auto trade = std::make_shared<TradeEvent>(symbol, timestamp, price, quantity, side);
            event_queue_->push(std::make_shared<std::shared_ptr<Event>>(std::static_pointer_cast<Event>(trade)));
            
            if (on_new_data_) {
                on_new_data_();
            }
        }
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON parse error in message: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;
    }
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
    if (it != latest_bars_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

double WebSocketDataHandler::getLatestBarValue(const std::string& symbol, const std::string& val_type) {
    auto it = latest_bars_map_.find(symbol);
    if (it != latest_bars_map_.end()) {
        if (val_type == "price" || val_type == "close") return it->second.close;
        if (val_type == "open") return it->second.open;
        if (val_type == "high") return it->second.high;
        if (val_type == "low") return it->second.low;
        if (val_type == "volume") return it->second.volume;
    }
    return 0.0;
}

std::vector<Bar> WebSocketDataHandler::getLatestBars(const std::string& symbol, int n) {
    std::vector<Bar> bars;
    auto it = latest_bars_map_.find(symbol);
    if (it != latest_bars_map_.end()) {
        bars.push_back(it->second);
    }
    return bars;
}