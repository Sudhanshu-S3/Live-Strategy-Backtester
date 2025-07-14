#include "strategy/OrderBookImbalanceStrategy.h"
#include <iostream>
#include <numeric> // For std::accumulate
#include <chrono>

// Helper to get a string timestamp
std::string getCurrentTimestampStr() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    return std::to_string(in_time_t);
}

OrderBookImbalanceStrategy::OrderBookImbalanceStrategy(
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> events_queue,
    const std::string& symbol,
    int lookback_levels,
    double buy_threshold,
    double sell_threshold
) : Strategy(events_queue),
    symbol_(symbol),
    lookback_levels_(lookback_levels),
    buy_threshold_(buy_threshold),
    sell_threshold_(sell_threshold) {}

void OrderBookImbalanceStrategy::onFill(const FillEvent& event) {
    // If we receive a fill event, it means our position state has changed.
    // A simple implementation assumes any fill opens or closes the one position we manage.
    if (event.direction == "BUY" || event.direction == "SELL") {
        position_active_ = true;
    } else if (event.direction == "EXIT_BUY" || event.direction == "EXIT_SELL") {
        position_active_ = false;
    }
}


void OrderBookImbalanceStrategy::onOrderBook(const OrderBookEvent& event) {
    if (event.book.symbol != symbol_) {
        return; // This event is not for us
    }

    double total_bid_volume = 0.0;
    double total_ask_volume = 0.0;

    // Calculate total volume for the top N levels
    for (int i = 0; i < lookback_levels_ && i < event.book.bids.size(); ++i) {
        total_bid_volume += event.book.bids[i].quantity;
    }
    for (int i = 0; i < lookback_levels_ && i < event.book.asks.size(); ++i) {
        total_ask_volume += event.book.asks[i].quantity;
    }

    if (total_bid_volume + total_ask_volume == 0) {
        return; // Avoid division by zero
    }

    // Calculate the imbalance ratio
    double imbalance_ratio = total_bid_volume / (total_bid_volume + total_ask_volume);
    
    std::string signal_type = "";

    // Generate signals based on the ratio and our current position
    if (!position_active_) {
        if (imbalance_ratio > buy_threshold_) {
            signal_type = "LONG";
            std::cout << "STRATEGY: Strong buy pressure detected! Ratio: " << imbalance_ratio << ". Sending LONG signal." << std::endl;
        } else if (imbalance_ratio < sell_threshold_) {
            signal_type = "SHORT";
            std::cout << "STRATEGY: Strong sell pressure detected! Ratio: " << imbalance_ratio << ". Sending SHORT signal." << std::endl;
        }
    } else { // position_active_ is true
        // Exit if pressure reverses or normalizes
        if (imbalance_ratio < sell_threshold_ || imbalance_ratio > buy_threshold_ || 
            (imbalance_ratio <= buy_threshold_ && imbalance_ratio >= sell_threshold_)) {
            signal_type = "EXIT";
            std::cout << "STRATEGY: Pressure changed. Ratio: " << imbalance_ratio << ". Sending EXIT signal." << std::endl;
        }
    }

    // If we generated a signal, create a SignalEvent and push it to the queue
    if (!signal_type.empty()) {
        auto signal = std::make_shared<SignalEvent>(symbol_, std::to_string(event.book.timestamp), signal_type);
        events_queue_->push(std::move(signal));
    }
};