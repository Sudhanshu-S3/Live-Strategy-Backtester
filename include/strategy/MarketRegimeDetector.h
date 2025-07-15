#ifndef MARKET_REGIME_DETECTOR_H
#define MARKET_REGIME_DETECTOR_H

#include <string>
#include <vector>
#include <map>
#include <deque> // For moving averages/volatility
#include "../data/DataHandler.h"
#include "../data/DataTypes.h"
#include "../event/EventQueue.h"

// Enum for different market regimes
enum class MarketRegime {
    NORMAL,
    HIGH_VOLATILITY,
    LOW_VOLATILITY,
    TRENDING_UP,
    TRENDING_DOWN,
    // Add more regimes as needed
};

// Helper to convert enum to string for logging
std::string marketRegimeToString(MarketRegime regime);

class MarketRegimeDetector {
public:
    MarketRegimeDetector(std::shared_ptr<DataHandler> data_handler, 
                         std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
                         const std::string& symbol,
                         int volatility_lookback_period = 20,
                         double high_volatility_threshold = 0.02, // e.g., daily std dev > 2%
                         double low_volatility_threshold = 0.005 // e.g., daily std dev < 0.5%
                         );

    // Call this method periodically with market data to update the regime
    void update(const MarketEvent& market_event);

    // Get the current detected market regime
    MarketRegime getCurrentRegime() const;

private:
    std::shared_ptr<DataHandler> data_handler_;
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue_;
    std::string symbol_;
    int volatility_lookback_period_;
    double high_volatility_threshold_;
    double low_volatility_threshold_;

    MarketRegime current_regime_ = MarketRegime::NORMAL;
    std::deque<double> recent_prices_; // To store prices for volatility calculation

    // Helper to calculate standard deviation for volatility
    double calculateStandardDeviation(const std::deque<double>& data) const;
    double calculateMean(const std::deque<double>& data) const;
};

#endif // MARKET_REGIME_DETECTOR_H