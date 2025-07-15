#ifndef MARKET_REGIME_DETECTOR_H
#define MARKET_REGIME_DETECTOR_H

#include <string>
#include <vector>
#include <deque>
#include "Strategy.h"
#include "../data/DataTypes.h"

class MarketRegimeDetector : public Strategy {
public:
    MarketRegimeDetector(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
                         std::shared_ptr<DataHandler> data_handler,
                         const std::string& symbol,
                         int volatility_lookback = 20,
                         int trend_lookback = 50,
                         double high_vol_threshold = 0.02,
                         double low_vol_threshold = 0.005,
                         double trend_threshold_pct = 0.5);

    void onMarket(const MarketEvent& event) override;
    void onTrade(const TradeEvent& event) override;
    void onOrderBook(const OrderBookEvent& event) override;
    void onFill(const FillEvent& event) override;

    MarketState getCurrentState() const;

private:
    int volatility_lookback_;
    int trend_lookback_;
    double high_vol_threshold_;
    double low_vol_threshold_;
    double trend_threshold_pct_;

    MarketState current_state_;
    std::deque<double> recent_prices_vol_;
    std::deque<double> recent_prices_trend_;

    void update_volatility();
    void update_trend();

    double calculateStandardDeviation(const std::deque<double>& data) const;
    double calculateMean(const std::deque<double>& data) const;
};

#endif