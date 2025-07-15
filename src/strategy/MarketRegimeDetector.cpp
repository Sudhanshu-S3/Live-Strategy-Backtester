#include "../../include/strategy/MarketRegimeDetector.h"
#include <numeric> // For std::accumulate
#include <cmath>   // For std::sqrt
#include <iostream>

std::string marketRegimeToString(MarketRegime regime) {
    switch (regime) {
        case MarketRegime::NORMAL: return "NORMAL";
        case MarketRegime::HIGH_VOLATILITY: return "HIGH_VOLATILITY";
        case MarketRegime::LOW_VOLATILITY: return "LOW_VOLATILITY";
        case MarketRegime::TRENDING_UP: return "TRENDING_UP";
        case MarketRegime::TRENDING_DOWN: return "TRENDING_DOWN";
        default: return "UNKNOWN";
    }
}

MarketRegimeDetector::MarketRegimeDetector(std::shared_ptr<DataHandler> data_handler, 
                                           std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
                                           const std::string& symbol,
                                           int volatility_lookback_period,
                                           double high_volatility_threshold,
                                           double low_volatility_threshold)
    : data_handler_(data_handler),
      event_queue_(event_queue),
      symbol_(symbol),
      volatility_lookback_period_(volatility_lookback_period),
      high_volatility_threshold_(high_volatility_threshold),
      low_volatility_threshold_(low_volatility_threshold) {}

void MarketRegimeDetector::update(const MarketEvent& market_event) {
    if (market_event.symbol != symbol_) {
        return; // Only process for the tracked symbol
    }

    double current_price = data_handler_->getLatestBarValue(symbol_, "price");
    if (current_price <= 0) {
        return; // No valid price
    }

    recent_prices_.push_back(current_price);
    if (recent_prices_.size() > volatility_lookback_period_) {
        recent_prices_.pop_front();
    }

    if (recent_prices_.size() < volatility_lookback_period_) {
        current_regime_ = MarketRegime::NORMAL; // Not enough data yet
        return;
    }

    double volatility = calculateStandardDeviation(recent_prices_);
    // std::cout << "DEBUG: Volatility for " << symbol_ << ": " << volatility << std::endl;

    MarketRegime new_regime = MarketRegime::NORMAL;

    if (volatility > high_volatility_threshold_) {
        new_regime = MarketRegime::HIGH_VOLATILITY;
    } else if (volatility < low_volatility_threshold_) {
        new_regime = MarketRegime::LOW_VOLATILITY;
    } else {
        new_regime = MarketRegime::NORMAL; // Default
    }

    // Basic trend detection (can be more sophisticated with MAs etc.)
    if (recent_prices_.size() >= 2) {
        if (recent_prices_.back() > recent_prices_.front() * 1.005) { // 0.5% increase over period
            new_regime = MarketRegime::TRENDING_UP;
        } else if (recent_prices_.back() < recent_prices_.front() * 0.995) { // 0.5% decrease over period
            new_regime = MarketRegime::TRENDING_DOWN;
        }
    }


    if (new_regime != current_regime_) {
        std::cout << "MARKET REGIME DETECTOR: Regime changed from " 
                  << marketRegimeToString(current_regime_) << " to " 
                  << marketRegimeToString(new_regime) << " for " << symbol_ << std::endl;
        current_regime_ = new_regime;
        // Optionally, send a RegimeChangeEvent to the event queue
        // events_->push(std::make_shared<RegimeChangeEvent>(symbol_, market_event.timestamp, new_regime));
    }
}

MarketRegime MarketRegimeDetector::getCurrentRegime() const {
    return current_regime_;
}

double MarketRegimeDetector::calculateMean(const std::deque<double>& data) const {
    if (data.empty()) {
        return 0.0;
    }
    double sum = 0.0;
    for (double val : data) {
        sum += val;
    }
    return sum / data.size();
}

double MarketRegimeDetector::calculateStandardDeviation(const std::deque<double>& data) const {
    if (data.size() < 2) {
        return 0.0;
    }
    double mean = calculateMean(data);
    double sq_sum = 0.0;
    for (double val : data) {
        sq_sum += (val - mean) * (val - mean);
    }
    return std::sqrt(sq_sum / data.size()); // Population standard deviation
}