#ifndef ORDER_BOOK_IMBALANCE_STRATEGY_H
#define ORDER_BOOK_IMBALANCE_STRATEGY_H

#include "Strategy.h"
#include <vector>

#include <chrono>


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


private:
    enum class PositionState { FLAT, LONG, SHORT };
    
    void generate_signal(OrderDirection direction);
    
    int lookback_levels_;
    double imbalance_threshold_; // Current dynamic threshold
    double base_imbalance_threshold_; // Base threshold value
    
    // Data structures for SIMD processing

    void onMarketRegimeChanged(const MarketRegimeChangedEvent& event) override;


    std::vector<float> bid_prices_f;
    std::vector<float> bid_qtys_f;
    std::vector<float> ask_prices_f;
    std::vector<float> ask_qtys_f;

    
    // State tracking
    long long last_signal_time_;
    long long signal_cooldown_ms_;
    PositionState current_position_;
};

#endif // ORDER_BOOK_IMBALANCE_STRATEGY_H

