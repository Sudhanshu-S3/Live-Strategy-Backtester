#ifndef ORDER_BOOK_IMBALANCE_STRATEGY_H
#define ORDER_BOOK_IMBALANCE_STRATEGY_H

#include "Strategy.h"

class OrderBookImbalanceStrategy : public Strategy {
public:
    OrderBookImbalanceStrategy(
        std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
        std::shared_ptr<DataHandler> data_handler,
        const std::string& symbol,
        int lookback_levels,
        double imbalance_threshold
    );

    void calculate_signals() override;

private:
    int lookback_levels_;
    double imbalance_threshold_;
    
    // STAGE 3: For SIMD
    std::vector<float> bid_prices_f;
    std::vector<float> bid_qtys_f;
    std::vector<float> ask_prices_f;
    std::vector<float> ask_qtys_f;
};

#endif