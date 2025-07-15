#ifndef SIMPLE_MOVING_AVERAGE_CROSSOVER_H
#define SIMPLE_MOVING_AVERAGE_CROSSOVER_H

#include "Strategy.h"
#include <string>
#include <deque>
#include <memory>

class SimpleMovingAverageCrossover : public Strategy {
public:
    SimpleMovingAverageCrossover(
        std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
        std::shared_ptr<DataHandler> data_handler,
        const std::string& name,
        const std::string& symbol,
        int short_window,
        int long_window
    );

    void onMarket(const MarketEvent& event) override;
    void onTrade(const TradeEvent& event) override;
    void onOrderBook(const OrderBookEvent& event) override;
    void onFill(const FillEvent& event) override;
    void onMarketRegimeChanged(const MarketRegimeChangedEvent& event) override;

private:
    void generate_signal(OrderDirection direction);
    double calculate_sma(int period);

    int short_window_;
    int long_window_;
    
    std::deque<double> prices_;
    double last_short_sma_ = 0.0;
    double last_long_sma_ = 0.0;
    
    enum class PositionState { FLAT, LONG, SHORT };
    PositionState current_position_;
};

#endif // SIMPLE_MOVING_AVERAGE_CROSSOVER_H