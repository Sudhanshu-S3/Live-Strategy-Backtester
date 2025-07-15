#include "../../include/strategy/SimpleMovingAverageCrossover.h"
#include <numeric>
#include <iostream>
#include <chrono>

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

    // Add the new price to our deque and maintain the size
    prices_.push_back(event.price);
    if (prices_.size() > long_window_) {
        prices_.pop_front();
    }

    // Wait until we have enough data for the long window SMA
    if (prices_.size() < long_window_) {
        return;
    }

    // Calculate the short and long simple moving averages
    double short_sma = calculate_sma(short_window_);
    double long_sma = calculate_sma(long_window_);

    // On the first tick with enough data, just record the SMAs and wait for the next tick
    // This prevents a false signal on initialization.
    if (last_long_sma_ == 0.0) {
        last_short_sma_ = short_sma;
        last_long_sma_ = long_sma;
        return;
    }

    // --- Corrected Trading Logic ---

    // Check for a crossover upwards
    if (short_sma > long_sma && last_short_sma_ <= last_long_sma_) {
        // If we are not already long, place a buy order.
        // This single signal handles both entering a new long position and reversing a short position.
        if (current_position_ != PositionState::LONG) {
            generate_signal(OrderDirection::BUY);
            current_position_ = PositionState::LONG;
        }
    } 
    // Check for a crossover downwards
    else if (short_sma < long_sma && last_short_sma_ >= last_long_sma_) {
        // If we are not already short, place a sell order.
        // This single signal handles both entering a new short position and reversing a long position.
        if (current_position_ != PositionState::SHORT) {
            generate_signal(OrderDirection::SELL);
            current_position_ = PositionState::SHORT;
        }
    }

    // Update the last SMA values for the next tick's comparison
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
    // Iterate from the end of the deque to get the most recent 'period' prices
    for (auto it = prices_.crbegin(); it != prices_.crbegin() + period; ++it) {
        sum += *it;
    }
    return sum / period;
}

void SimpleMovingAverageCrossover::generate_signal(OrderDirection direction) {
    // Assuming SignalEvent and OrderDirection are defined in included headers
    // and the base Strategy class provides 'name', 'symbol', and 'event_queue_'
    long long timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto signal = std::make_shared<SignalEvent>(name, symbol, timestamp, direction, 0.0, 1.0);
    event_queue_->push(std::make_shared<std::shared_ptr<Event>>(signal));
};