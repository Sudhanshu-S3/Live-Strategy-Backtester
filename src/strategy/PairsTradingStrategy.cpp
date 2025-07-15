#include "../../include/strategy/PairsTradingStrategy.h"
#include <numeric>
#include <iostream>

void PairsTradingStrategy::onTrade(const TradeEvent& event) {
    if (event.symbol == symbol1_) {
        prices1_.push_back(event.price);
        if (prices1_.size() > lookback_) prices1_.pop_front();
    } else if (event.symbol == symbol2_) {
        prices2_.push_back(event.price);
        if (prices2_.size() > lookback_) prices2_.pop_front();
    }

    if (prices1_.size() == lookback_ && prices2_.size() == lookback_) {
        calculate_spread();
    }
}

void PairsTradingStrategy::calculate_spread() {
    double spread = prices1_.back() - prices2_.back(); // Simple spread, could be log ratio
    spread_history_.push_back(spread);
    if (spread_history_.size() > lookback_) {
        spread_history_.pop_front();
    } else {
        return; // Not enough history to calculate stats
    }

    double mean = std::accumulate(spread_history_.begin(), spread_history_.end(), 0.0) / spread_history_.size();
    double sq_sum = std::inner_product(spread_history_.begin(), spread_history_.end(), spread_history_.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / spread_history_.size() - mean * mean);

    if (stdev < 1e-6) return;

    double z_score = (spread - mean) / stdev;

    if (z_score < -entry_threshold_z_ && !in_long_spread_) {
        // Spread is low, buy spread (buy symbol1, sell symbol2)
        sendSignal(symbol1_, OrderDirection::BUY, 1.0); // Quantity is placeholder
        sendSignal(symbol2_, OrderDirection::SELL, 1.0);
        in_long_spread_ = true;
        in_short_spread_ = false;
    } else if (z_score > entry_threshold_z_ && !in_short_spread_) {
        // Spread is high, sell spread (sell symbol1, buy symbol2)
        sendSignal(symbol1_, OrderDirection::SELL, 1.0);
        sendSignal(symbol2_, OrderDirection::BUY, 1.0);
        in_short_spread_ = true;
        in_long_spread_ = false;
    } else if (std::abs(z_score) < exit_threshold_z_) {
        // Reverted to mean, close positions
        if (in_long_spread_) {
            sendSignal(symbol1_, OrderDirection::SELL, 1.0);
            sendSignal(symbol2_, OrderDirection::BUY, 1.0);
            in_long_spread_ = false;
        }
        if (in_short_spread_) {
            sendSignal(symbol1_, OrderDirection::BUY, 1.0);
            sendSignal(symbol2_, OrderDirection::SELL, 1.0);
            in_short_spread_ = false;
        }
    }
}