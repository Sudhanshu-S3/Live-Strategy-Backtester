#ifndef PAIRS_TRADING_STRATEGY_H
#define PAIRS_TRADING_STRATEGY_H

#include "Strategy.h"
#include <deque>
#include <string>
#include <vector>
#include <cmath>
#include <stdexcept>

class PairsTradingStrategy : public Strategy {
public:
    PairsTradingStrategy(
        std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
        std::shared_ptr<DataHandler> data_handler,
        const std::vector<std::string>& symbols,
        int lookback,
        double entry_threshold_z,
        double exit_threshold_z
    ) : Strategy(event_queue, data_handler, "PAIRS_TRADING_" + symbols[0] + "_" + symbols[1], symbols[0] + "_" + symbols[1]),
        lookback_(lookback),
        entry_threshold_z_(entry_threshold_z),
        exit_threshold_z_(exit_threshold_z) {
        if (symbols.size() != 2) {
            throw std::invalid_argument("PairsTradingStrategy requires exactly two symbols.");
        }
        symbol1_ = symbols[0];
        symbol2_ = symbols[1];
    }

    // Implement pure virtual functions from base class
    void onMarket(const MarketEvent& event) override;
    void onTrade(const TradeEvent& event) override;
    void onOrderBook(const OrderBookEvent& event) override;
    void onFill(const FillEvent& event) override;

private:
    void calculate_spread();

    std::string symbol1_, symbol2_;
    int lookback_;
    double entry_threshold_z_;
    double exit_threshold_z_;

    std::deque<double> prices1_, prices2_;
    std::deque<double> spread_history_;
    
    bool in_long_spread_ = false;
    bool in_short_spread_ = false;
};

#endif // PAIRS_TRADING_STRATEGY_H