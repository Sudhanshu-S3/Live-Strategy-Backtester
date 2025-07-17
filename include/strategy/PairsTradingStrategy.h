#ifndef PAIRS_TRADING_STRATEGY_H
#define PAIRS_TRADING_STRATEGY_H

#include "Strategy.h"
#include <deque>
#include <string>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <map>
#include <numeric> // Add this for std::accumulate and std::inner_product

class PairsTradingStrategy : public Strategy {
public:
    PairsTradingStrategy(
        std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
        std::shared_ptr<DataHandler> data_handler,
        const std::string& name,
        const std::string& symbol_a,
        const std::string& symbol_b,
        int window,
        double z_score_threshold
    );

    void onMarket(const MarketEvent& event) override;
    void onTrade(const TradeEvent& event) override;
    void onOrderBook(const OrderBookEvent& event) override;
    void onFill(const FillEvent& event) override;

private:
    enum class PositionState { FLAT, LONG_PAIR, SHORT_PAIR };
    
    void generate_signal(const std::string& signal_symbol, OrderDirection direction);
    
    std::string symbol_a_;
    std::string symbol_b_;
    int window_;
    double z_score_threshold_;
    std::map<std::string, double> latest_prices_;
    std::deque<double> ratio_history_;
    PositionState current_position_;
};

#endif // PAIRS_TRADING_STRATEGY_H