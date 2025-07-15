#ifndef ORDER_BOOK_IMBALANCE_STRATEGY_H
#define ORDER_BOOK_IMBALANCE_STRATEGY_H

#include "Strategy.h"
#include <vector>
#include "../../include/data/DataTypes.h"
#include "../../include/event/Event.h"

class OrderBookImbalanceStrategy : public Strategy {
public:
    OrderBookImbalanceStrategy(
        std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
        std::shared_ptr<DataHandler> data_handler,
        const std::string& symbol,
        int lookback_levels,
        double imbalance_threshold
    );

    void onMarket(const MarketEvent& event) override;
    void onTrade(const TradeEvent& event) override;
    void onOrderBook(const OrderBookEvent& event) override;
    void onFill(const FillEvent& event) override;
    void onMarketRegimeChanged(const MarketRegimeChangedEvent& event) override;

private:
    void generate_signal(OrderDirection direction, double strength);

    int lookback_levels_;
    double base_imbalance_threshold_;
    MarketState current_market_state_;

    long long last_update_timestamp_ = 0;
    
    // STAGE 3: For SIMD
    std::vector<float> bid_prices_f;
    std::vector<float> bid_qtys_f;
    std::vector<float> ask_prices_f;
    std::vector<float> ask_qtys_f;
};

#endif