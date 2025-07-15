#include "../../include/data/WebSocketDataHandler.h"
#include "../../include/event/Event.h"
#include <iostream>
#include <nlohmann/json.hpp> // Added for JSON parsing
#include <chrono> // Added for timestamp

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

WebSocketDataHandler::WebSocketDataHandler(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue, const std::string& ws_uri, const std::vector<std::string>& symbols)
    : event_queue_(event_queue), uri_(ws_uri), symbols_(symbols), ws_client_(new client()) {
    
    ws_client_->init_asio();
    ws_client_->set_message_handler(bind(&WebSocketDataHandler::on_message, this, ::_1, ::_2));

    ws_thread_ = std::thread(&WebSocketDataHandler::run, this);
}

WebSocketDataHandler::~WebSocketDataHandler() {
    finished_ = true;
    
    if (ws_thread_.joinable()) {
        // More graceful shutdown needed here, like closing the websocket connection
        ws_client_->stop();
        ws_thread_.join();
    }
}

void WebSocketDataHandler::run() {
    try {
        websocketpp::lib::error_code ec;
        client::connection_ptr con = ws_client_->get_connection(uri_, ec);
        if (ec) {
            std::cerr << "Could not create connection because: " << ec.message() << std::endl;
            return;
        }

        ws_client_->connect(con);
        ws_client_->run();
    } catch (websocketpp::exception const & e) {
        std::cerr << "WebSocket++ exception: " << e.what() << std::endl;
    }
}

void WebSocketDataHandler::on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) {
    try {
        auto json_msg = nlohmann::json::parse(msg->get_payload());

        // This is a hypothetical trade message format from an exchange like Binance
        if (json_msg.contains("e") && json_msg["e"] == "trade") {
            std::string symbol = json_msg["s"];
            long long timestamp = json_msg["T"];
            double price = std::stod(json_msg["p"].get<std::string>());
            double quantity = std::stod(json_msg["q"].get<std::string>());
            std::string side = json_msg["m"] ? "SELL" : "BUY"; // true if maker is seller

            auto trade = std::make_shared<TradeEvent>(symbol, timestamp, price, quantity, side);
            trade->timestamp_received = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            event_queue_->push(trade);
        }
        // Add parsers for other message types like order book updates here
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON parse error in WebSocket message: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error processing WebSocket message: " << e.what() << std::endl;
    }
}

void WebSocketDataHandler::updateBars() {
    // This method is called by the backtester's main loop.
    // In this implementation, the WebSocket thread pushes events directly
    // to the main event queue, so this method can be empty.
}

bool WebSocketDataHandler::isFinished() const {
    // For a live feed, this will likely always be false unless there's a disconnection.
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
        if (val_type == "price" || val_type == "close") return it->second.close_price;
        if (val_type == "open") return it->second.open_price;
        if (val_type == "high") return it->second.high_price;
        if (val_type == "low") return it->second.low_price;
        if (val_type == "volume") return it->second.volume;
    }
    return 0.0; // Or throw an exception
}

std::vector<Bar> WebSocketDataHandler::getLatestBars(const std::string& symbol, int n) {
    // This would require storing a history of bars. For now, returning latest if available.
    std::vector<Bar> bars;
    auto it = latest_bars_map_.find(symbol);
    if (it != latest_bars_map_.end()) {
        bars.push_back(it->second);
    }
    return bars;
} 