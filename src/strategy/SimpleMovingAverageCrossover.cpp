#include "../../include/strategy/SimpleMovingAverageCrossover.h"
#include <numeric>
#include <iostream>

SimpleMovingAverageCrossover::SimpleMovingAverageCrossover(
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
    std::shared_ptr<DataHandler> data_handler,
    const std::string& name,
    const std::string& symbol,
    int short_window,
    int long_window
) : Strategy(event_queue, data_handler, name, symbol),
    short_window_(short_window),
    long_window_(long_window),
    current_position_(PositionState::FLAT) {}

void SimpleMovingAverageCrossover::onMarket(const MarketEvent& event) {
    if (event.symbol != symbol) return;

    prices_.push_back(event.price);
    if (prices_.size() > long_window_) {
        prices_.pop_front();
    }

    if (prices_.size() < long_window_) {
        return;
    }

    double short_sma = calculate_sma(short_window_);
    double long_sma = calculate_sma(long_window_);

    if (short_sma == 0.0 || long_sma == 0.0) return;

    // --- Trading Logic ---
    if (short_sma > long_sma && last_short_sma_ <= last_long_sma_) { // Crossover up
        if (current_position_ == PositionState::SHORT) {
            generate_signal(OrderDirection::NONE); // Exit short
        }
        generate_signal(OrderDirection::BUY);
        current_position_ = PositionState::LONG;
    } else if (short_sma < long_sma && last_short_sma_ >= last_long_sma_) { // Crossover down
        if (current_position_ == PositionState::LONG) {
            generate_signal(OrderDirection::NONE); // Exit long
        }
        generate_signal(OrderDirection::SELL);
        current_position_ = PositionState::SHORT;
    }
    
    last_short_sma_ = short_sma;
    last_long_sma_ = long_sma;
}

void SimpleMovingAverageCrossover::onTrade(const TradeEvent& event) {}
void SimpleMovingAverageCrossover::onOrderBook(const OrderBookEvent& event) {}
void SimpleMovingAverageCrossover::onFill(const FillEvent& event) {}
void SimpleMovingAverageCrossover::onMarketRegimeChanged(const MarketRegimeChangedEvent& event) {}


double SimpleMovingAverageCrossover::calculate_sma(int period) {
    if (prices_.size() < period) {
        return 0.0;
    }
    
    double sum = 0.0;
    // Iterate from the end of the deque
    for (auto it = prices_.end() - period; it != prices_.end(); ++it) {
        sum += *it;
    }
    return sum / period;
}

void SimpleMovingAverageCrossover::generate_signal(OrderDirection direction) {
    long long timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto signal = std::make_shared<SignalEvent>(name, symbol, timestamp, direction, 0.0, 1.0);
    event_queue_->push(signal);
}
