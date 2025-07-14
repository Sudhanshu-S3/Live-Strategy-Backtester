#include "../../include/strategy/PairTradingStrategy.h"
#include <numeric>
#include <cmath>
#include <iostream>

PairTradingStrategy::PairTradingStrategy(
    std::string symbol_a, 
    std::string symbol_b, 
    int window, 
    double z_score_threshold,
    std::shared_ptr<DataHandler> data_handler)
    : symbol_a(std::move(symbol_a)), 
      symbol_b(std::move(symbol_b)), 
      window(window), 
      z_score_threshold(z_score_threshold),
      data_handler(data_handler) {
        latest_prices[this->symbol_a] = 0.0;
        latest_prices[this->symbol_b] = 0.0;
}

void PairTradingStrategy::generateSignals(
    const MarketEvent& event, 
    std::queue<std::shared_ptr<Event>>& event_queue
) {
    // 1. Get the latest bar and check if it's valid
    auto latest_bar_opt = data_handler->getLatestBar(event.symbol);
    if (!latest_bar_opt) {
        return; // No bar data for this symbol yet, do nothing.
    }
    // Update the internal price map
    latest_prices[event.symbol] = latest_bar_opt->close;

    // 2. Check if we have prices for both assets
    if (latest_prices[symbol_a] <= 0.0 || latest_prices[symbol_b] <= 0.0) {
        return; // Not ready yet, wait for both prices
    }

    // 3. Calculate the new price ratio and add to history
    double current_ratio = latest_prices[symbol_a] / latest_prices[symbol_b];
    ratio_history.push_back(current_ratio);

    if (ratio_history.size() < window) {
        return; // Need more data
    }
    
    // 4. Calculate moving average and standard deviation
    auto first = ratio_history.end() - window;
    auto last = ratio_history.end();
    
    double sum = std::accumulate(first, last, 0.0);
    double mean = sum / window;
    
    double sq_sum = std::inner_product(first, last, first, 0.0);
    double std_dev = std::sqrt(sq_sum / window - mean * mean);

    if (std_dev < 1e-6) return; // Avoid division by zero

    // 5. Calculate the Z-score and generate signals
    double z_score = (current_ratio - mean) / std_dev;
    auto timestamp = event.timestamp;

    if (current_position == PositionState::NONE) {
        if (z_score > z_score_threshold) { // Short the pair
            event_queue.push(std::make_shared<SignalEvent>(symbol_a, timestamp, "SHORT"));
            event_queue.push(std::make_shared<SignalEvent>(symbol_b, timestamp, "LONG"));
            current_position = PositionState::SHORT;
        } else if (z_score < -z_score_threshold) { // Long the pair
            event_queue.push(std::make_shared<SignalEvent>(symbol_a, timestamp, "LONG"));
            event_queue.push(std::make_shared<SignalEvent>(symbol_b, timestamp, "SHORT"));
            current_position = PositionState::LONG;
        }
    }
    else if ((current_position == PositionState::SHORT && z_score < 0.5) ||
             (current_position == PositionState::LONG && z_score > -0.5)) { // Exit logic
        event_queue.push(std::make_shared<SignalEvent>(symbol_a, timestamp, "EXIT"));
        event_queue.push(std::make_shared<SignalEvent>(symbol_b, timestamp, "EXIT"));
        current_position = PositionState::NONE;
    }
}