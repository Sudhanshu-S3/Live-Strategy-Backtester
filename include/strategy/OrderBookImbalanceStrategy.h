#ifndef ORDER_BOOK_IMBALANCE_STRATEGY_H
#define ORDER_BOOK_IMBALANCE_STRATEGY_H

#include "Strategy.h"
#include <vector>
<<<<<<< HEAD
#include <chrono>
=======
#include "../../include/data/DataTypes.h"
#include "../../include/event/Event.h"
>>>>>>> ef82a6ae559d39c2be7a0dee4c6355537669c2a5

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
<<<<<<< HEAD

private:
    enum class PositionState { FLAT, LONG, SHORT };
    
    void generate_signal(OrderDirection direction);
    
    int lookback_levels_;
    double base_imbalance_threshold_;
    
    // Data structures for SIMD processing
=======
    void onMarketRegimeChanged(const MarketRegimeChangedEvent& event) override;

private:
    void generate_signal(OrderDirection direction, double strength);

    int lookback_levels_;
    double base_imbalance_threshold_;
    MarketState current_market_state_;

    long long last_update_timestamp_ = 0;
    
    // STAGE 3: For SIMD
>>>>>>> ef82a6ae559d39c2be7a0dee4c6355537669c2a5
    std::vector<float> bid_prices_f;
    std::vector<float> bid_qtys_f;
    std::vector<float> ask_prices_f;
    std::vector<float> ask_qtys_f;
<<<<<<< HEAD
    
    // State tracking
    long long last_signal_time_;
    long long signal_cooldown_ms_;
    PositionState current_position_;
};

#endif // ORDER_BOOK_IMBALANCE_STRATEGY_H
=======
};

#endif
>>>>>>> ef82a6ae559d39c2be7a0dee4c6355537669c2a5
