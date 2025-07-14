#include "../../include/strategy/PairTradingStrategy.h"
#include <numeric>
#include <cmath>
#include <iostream>

PairTradingStrategy::PairTradingStrategy(
    std::string symbol_a, 
    std::string symbol_b, 
    int window, 
    double z_score_threshold,
    std::shared_ptr<DataHandler> data_handler,
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> events_queue)
    : Strategy(events_queue),
      symbol_a(std::move(symbol_a)), 
      symbol_b(std::move(symbol_b)), 
      window(window), 
      z_score_threshold(z_score_threshold),
      data_handler(data_handler) {
        latest_prices[this->symbol_a] = 0.0;
        latest_prices[this->symbol_b] = 0.0;
}

void PairTradingStrategy::onMarket(
    const MarketEvent& event
) {
    if (event.symbol != symbol_a && event.symbol != symbol_b) {
        return;
    }

    // Update the latest prices from the data handler
    auto bar_opt = data_handler->getLatestBar(event.symbol);
    if (!bar_opt) {
        return; // No bar data available
    }
    latest_prices[event.symbol] = bar_opt->close;

    // Ensure we have prices for both symbols
    if (latest_prices[symbol_a] == 0.0 || latest_prices[symbol_b] == 0.0) {
        return;
    }

    // Calculate the new price ratio and add it to our history
    double price_a = latest_prices[symbol_a];
    double price_b = latest_prices[symbol_b];
    if (price_b == 0) return; // Avoid division by zero
    double ratio = price_a / price_b;
    ratio_history.push_back(ratio);

    // Don't generate signals until we have enough data
    if (ratio_history.size() < window) {
        return;
    }

    // Keep the history to the size of the window
    if (ratio_history.size() > window) {
        ratio_history.erase(ratio_history.begin());
    }

    // Calculate the z-score
    double sum = std::accumulate(ratio_history.begin(), ratio_history.end(), 0.0);
    double mean = sum / window;
    double sq_sum = std::inner_product(ratio_history.begin(), ratio_history.end(), ratio_history.begin(), 0.0);
    double std_dev = std::sqrt(sq_sum / window - mean * mean);
    if (std_dev < 1e-8) return; // Avoid division by zero if standard deviation is too small
    double z_score = (ratio - mean) / std_dev;

    auto timestamp = event.timestamp;

    // --- Trading Logic ---
    if (z_score > z_score_threshold && current_position != PositionState::SHORT) {
        // Short the pair (sell A, buy B)
        if (current_position == PositionState::LONG) {
            // Close existing long position first
            events_queue_->push(std::make_shared<SignalEvent>(symbol_a, timestamp, "EXIT"));
            events_queue_->push(std::make_shared<SignalEvent>(symbol_b, timestamp, "EXIT"));
        }
        events_queue_->push(std::make_shared<SignalEvent>(symbol_a, timestamp, "SELL"));
        events_queue_->push(std::make_shared<SignalEvent>(symbol_b, timestamp, "BUY"));
        current_position = PositionState::SHORT;
    } else if (z_score < -z_score_threshold && current_position != PositionState::LONG) {
        // Long the pair (buy A, sell B)
        if (current_position == PositionState::SHORT) {
            // Close existing short position first
            events_queue_->push(std::make_shared<SignalEvent>(symbol_a, timestamp, "EXIT"));
            events_queue_->push(std::make_shared<SignalEvent>(symbol_b, timestamp, "EXIT"));
        }
        events_queue_->push(std::make_shared<SignalEvent>(symbol_a, timestamp, "BUY"));
        events_queue_->push(std::make_shared<SignalEvent>(symbol_b, timestamp, "SELL"));
        current_position = PositionState::LONG;
    } else if (std::abs(z_score) < 0.5 && current_position != PositionState::NONE) {
        // Exit position if z-score approaches zero (the mean)
        events_queue_->push(std::make_shared<SignalEvent>(symbol_a, timestamp, "EXIT"));
        events_queue_->push(std::make_shared<SignalEvent>(symbol_b, timestamp, "EXIT"));
        current_position = PositionState::NONE;
    }
}