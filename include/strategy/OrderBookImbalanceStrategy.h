#ifndef ORDER_BOOK_IMBALANCE_STRATEGY_H
#define ORDER_BOOK_IMBALANCE_STRATEGY_H

#include "Strategy.h"
#include "../data/DataTypes.h"
#include <string>
#include <queue>
#include <memory>

class OrderBookImbalanceStrategy : public Strategy {
public:
    OrderBookImbalanceStrategy(
        std::shared_ptr<std::queue<std::shared_ptr<Event>>> events_queue,
        const std::string& symbol,
        int lookback_levels,
        double buy_threshold,
        double sell_threshold
    );

    // Base class methods for handling HFT events
    void onOrderBook(const OrderBookEvent& event);

    // We also need to know when our orders are filled to update our state
    void onFill(const FillEvent& event);

private:
    std::string symbol_;
    int lookback_levels_;
    double buy_threshold_;
    double sell_threshold_;

    // State tracking
    bool position_active_ = false; // Is there any open position?
};

#endif // ORDER_BOOK_IMBALANCE_STRATEGY_H