#include "../../include/strategy/SimpleMovingAverageCrossover.h"
#include <numeric>
#include <iostream>

SimpleMovingAverageCrossover::SimpleMovingAverageCrossover(
    std::string symbol, 
    int short_window, 
    int long_window,
    std::shared_ptr<DataHandler> data_handler
) : symbol(std::move(symbol)), 
    short_window(short_window), 
    long_window(long_window),
    data_handler(std::move(data_handler)) {}

double SimpleMovingAverageCrossover::calculate_sma(int period) {
    if (prices.size() < period) {
        return 0.0;
    }
    double sum = std::accumulate(prices.end() - period, prices.end(), 0.0);
    return sum / period;
}

void SimpleMovingAverageCrossover::generateSignals(
    const MarketEvent& event, 
    std::queue<std::shared_ptr<Event>>& event_queue
) {
    if (event.symbol != this->symbol) {
        return;
    }

    auto latest_bar_opt = data_handler->getLatestBar(this->symbol);
    if (!latest_bar_opt) {
        return;
    }
    const auto& bar = *latest_bar_opt;
    prices.push_back(bar.close);

    if (prices.size() < long_window) {
        return;
    }

    double short_sma = calculate_sma(short_window);
    double long_sma = calculate_sma(long_window);

    auto timestamp = bar.timestamp;

    if (current_position == PositionState::NONE) {
        if (short_sma > long_sma) {
            event_queue.push(std::make_shared<SignalEvent>(this->symbol, timestamp, "LONG"));
            current_position = PositionState::LONG;
        } else if (short_sma < long_sma) {
            event_queue.push(std::make_shared<SignalEvent>(this->symbol, timestamp, "SHORT"));
            current_position = PositionState::SHORT;
        }
    }
