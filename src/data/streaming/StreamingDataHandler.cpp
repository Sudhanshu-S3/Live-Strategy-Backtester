#include "streaming/StreamingDataHandler.h"
#include "../../../include/event/MarketEvent.h"
#include <chrono>
#include <random>
#include <iostream>

StreamingDataHandler::StreamingDataHandler(EventQueue& events, const std::vector<std::string>& symbols)
    : events_(events), symbols_(symbols), continue_streaming_(true) {
    data_thread_ = std::thread(&StreamingDataHandler::stream_data, this);
}

StreamingDataHandler::~StreamingDataHandler() {
    continue_streaming_ = false;
    if (data_thread_.joinable()) {
        data_thread_.join();
    }
}

void StreamingDataHandler::continue_backtest() {
    // For live streaming, this might not be needed, or could signal to continue processing
}

void StreamingDataHandler::update_bars() {
    // Data is pushed to the queue by the streaming thread
}

void StreamingDataHandler::stream_data() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(99.0, 101.0);

    while (continue_streaming_) {
        for (const auto& symbol : symbols_) {
            double price = dis(gen);
            Bar bar(symbol, "2025-07-15T12:00:00Z", price, price, price, price, 100);
            events_.push(std::make_shared<MarketEvent>(bar));
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
