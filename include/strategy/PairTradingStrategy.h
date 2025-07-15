#ifndef PAIR_TRADING_STRATEGY_H
#define PAIR_TRADING_STRATEGY_H

#include "Strategy.h"
#include <string>
#include <vector>
#include <map>
#include <deque>

class PairTradingStrategy : public Strategy {
public:
    PairTradingStrategy(
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
    void onMarketRegimeChanged(const MarketRegimeChangedEvent& event) override;

private:
    void generate_signal(const std::string& signal_symbol, OrderDirection direction);

    enum class PositionState { FLAT, LONG_PAIR, SHORT_PAIR };

    std::string symbol_a_;
    std::string symbol_b_;
    int window_;
    double z_score_threshold_;
    double base_z_score_threshold_;

    PositionState current_position_;
    std::map<std::string, double> latest_prices_;
    std::deque<double> ratio_history_;
};

#endif // PAIR_TRADING_STRATEGY_H