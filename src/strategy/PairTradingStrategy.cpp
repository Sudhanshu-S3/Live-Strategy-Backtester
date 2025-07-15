#include "../../include/strategy/PairTradingStrategy.h"
#include <numeric>
#include <cmath>
#include <iostream>

PairTradingStrategy::PairTradingStrategy(
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
    std::shared_ptr<DataHandler> data_handler,
    const std::string& name,
    const std::string& symbol_a,
    const std::string& symbol_b,
    int window,
    double z_score_threshold
) : Strategy(event_queue, data_handler, name, symbol_a), // Note: base strategy is for one symbol
    symbol_a_(symbol_a),
    symbol_b_(symbol_b),
    window_(window),
    z_score_threshold_(z_score_threshold),
    current_position_(PositionState::NONE)
{
    latest_prices_[symbol_a_] = 0.0;
    latest_prices_[symbol_b_] = 0.0;
}

void PairTradingStrategy::onMarket(const MarketEvent& event) {
    if (event.symbol != symbol_a_ && event.symbol != symbol_b_) {
        return;
    }

    latest_prices_[event.symbol] = event.price;

    if (latest_prices_[symbol_a_] <= 0.0 || latest_prices_[symbol_b_] <= 0.0) {
        return;
    }

    double price_a = latest_prices_[symbol_a_];
    double price_b = latest_prices_[symbol_b_];
    double ratio = price_a / price_b;
    ratio_history_.push_back(ratio);

    if (ratio_history_.size() > window_) {
        ratio_history_.pop_front();
    }

    if (ratio_history_.size() < window_) {
        return;
    }

    double sum = std::accumulate(ratio_history_.begin(), ratio_history_.end(), 0.0);
    double mean = sum / window_;
    double sq_sum = std::inner_product(ratio_history_.begin(), ratio_history_.end(), ratio_history_.begin(), 0.0);
    double std_dev = std::sqrt(sq_sum / window_ - mean * mean);

    if (std_dev < 1e-8) return;
    double z_score = (ratio - mean) / std_dev;

    // --- Trading Logic ---
    if (z_score > z_score_threshold_ && current_position_ != PositionState::SHORT_PAIR) {
        // Short the pair (sell A, buy B)
        generate_signal(symbol_a_, OrderDirection::SELL);
        generate_signal(symbol_b_, OrderDirection::BUY);
        current_position_ = PositionState::SHORT_PAIR;
    } else if (z_score < -z_score_threshold_ && current_position_ != PositionState::LONG_PAIR) {
        // Long the pair (buy A, sell B)
        generate_signal(symbol_a_, OrderDirection::BUY);
        generate_signal(symbol_b_, OrderDirection::SELL);
        current_position_ = PositionState::LONG_PAIR;
    } else if (std::abs(z_score) < 0.5 && current_position_ != PositionState::FLAT) {
        // Exit position
        generate_signal(symbol_a_, OrderDirection::NONE); // Assuming NONE direction signals exit
        generate_signal(symbol_b_, OrderDirection::NONE);
        current_position_ = PositionState::FLAT;
    }
}

void PairTradingStrategy::onTrade(const TradeEvent& event) {}
void PairTradingStrategy::onOrderBook(const OrderBookEvent& event) {}
void PairTradingStrategy::onFill(const FillEvent& event) {}
void PairTradingStrategy::onMarketRegimeChanged(const MarketRegimeChangedEvent& event) {
    market_state_ = event.new_state;
    // Potentially adjust z-score threshold based on volatility
    if (market_state_.volatility == VolatilityLevel::HIGH) {
        z_score_threshold_ = 1.5 * base_z_score_threshold_;
    } else {
        z_score_threshold_ = base_z_score_threshold_;
    }
}

void PairTradingStrategy::generate_signal(const std::string& signal_symbol, OrderDirection direction) {
    long long timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto signal = std::make_shared<SignalEvent>(name, signal_symbol, timestamp, direction, 0.0, 1.0);
    event_queue_->push(signal);
}