#include "../../include/strategy/MarketRegimeDetector.h"
#include <numeric>
#include <cmath>
#include <iostream>

MarketRegimeDetector::MarketRegimeDetector(
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
    std::shared_ptr<DataHandler> data_handler,
    const std::string& symbol,
    int volatility_lookback,
    int trend_lookback,
    double high_vol_threshold,
    double low_vol_threshold,
    double trend_threshold_pct
) : Strategy(event_queue, data_handler, "MARKET_REGIME_DETECTOR", symbol),
    volatility_lookback_(volatility_lookback),
    trend_lookback_(trend_lookback),
    high_vol_threshold_(high_vol_threshold),
    low_vol_threshold_(low_vol_threshold),
    trend_threshold_pct_(trend_threshold_pct) {}

void MarketRegimeDetector::onMarket(const MarketEvent& event) {
    if (event.symbol != symbol) return;

    recent_prices_vol_.push_back(event.price);
    if (recent_prices_vol_.size() > volatility_lookback_) {
        recent_prices_vol_.pop_front();
    }

    recent_prices_trend_.push_back(event.price);
    if (recent_prices_trend_.size() > trend_lookback_) {
        recent_prices_trend_.pop_front();
    }

    MarketState old_state = current_state_;
    update_volatility();
    update_trend();

    if (old_state.volatility != current_state_.volatility || old_state.trend != current_state_.trend) {
        std::cout << "Market regime changed for " << symbol 
                  << ": Volatility=" << static_cast<int>(current_state_.volatility) 
                  << ", Trend=" << static_cast<int>(current_state_.trend) << std::endl;
        auto regime_event = std::make_shared<MarketRegimeChangedEvent>(current_state_);
        event_queue_->push(regime_event);
    }
}

void MarketRegimeDetector::onTrade(const TradeEvent& event) {
    if (event.symbol != symbol) return;
    // Potentially use trade data as well
    onMarket(MarketEvent(event.symbol, event.timestamp, event.price));
}

void MarketRegimeDetector::onOrderBook(const OrderBookEvent& event) {
    // Not used for this strategy
}

void MarketRegimeDetector::onFill(const FillEvent& event) {
    // Not used for this strategy
}

MarketState MarketRegimeDetector::getCurrentState() const {
    return current_state_;
}

void MarketRegimeDetector::update_volatility() {
    if (recent_prices_vol_.size() < 2) return;
    
    double mean = calculateMean(recent_prices_vol_);
    double stddev = calculateStandardDeviation(recent_prices_vol_);
    double realized_vol = stddev / mean;
    current_state_.volatility_value = realized_vol;

    if (realized_vol > high_vol_threshold_) {
        current_state_.volatility = VolatilityLevel::HIGH;
    } else if (realized_vol < low_vol_threshold_) {
        current_state_.volatility = VolatilityLevel::LOW;
    } else {
        current_state_.volatility = VolatilityLevel::NORMAL;
    }
}

void MarketRegimeDetector::update_trend() {
    if (recent_prices_trend_.size() < trend_lookback_) return;

    double oldest_price = recent_prices_trend_.front();
    double newest_price = recent_prices_trend_.back();
    double pct_change = (newest_price - oldest_price) / oldest_price;

    if (pct_change > trend_threshold_pct_) {
        current_state_.trend = TrendDirection::TRENDING_UP;
    } else if (pct_change < -trend_threshold_pct_) {
        current_state_.trend = TrendDirection::TRENDING_DOWN;
    } else {
        current_state_.trend = TrendDirection::SIDEWAYS;
    }
}

double MarketRegimeDetector::calculateMean(const std::deque<double>& data) const {
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    return sum / data.size();
}

double MarketRegimeDetector::calculateStandardDeviation(const std::deque<double>& data) const {
    if (data.size() < 2) return 0.0;
    double mean = calculateMean(data);
    double sq_sum = std::inner_product(data.begin(), data.end(), data.begin(), 0.0);
    return std::sqrt(sq_sum / data.size() - mean * mean);
}

std::string marketStateToString(const MarketState& state) {
    std::string vol_str, trend_str;
    switch(state.volatility) {
        case VolatilityLevel::LOW: vol_str = "LOW"; break;
        case VolatilityLevel::NORMAL: vol_str = "NORMAL"; break;
        case VolatilityLevel::HIGH: vol_str = "HIGH"; break;
    }
    switch(state.trend) {
        case TrendDirection::SIDEWAYS: trend_str = "SIDEWAYS"; break;
        case TrendDirection::TRENDING_UP: trend_str = "UP"; break;
        case TrendDirection::TRENDING_DOWN: trend_str = "DOWN"; break;
    }
    return "Vol: " + vol_str + ", Trend: " + trend_str;
}