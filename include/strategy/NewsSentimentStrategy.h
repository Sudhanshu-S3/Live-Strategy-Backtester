#ifndef NEWS_SENTIMENT_STRATEGY_H
#define NEWS_SENTIMENT_STRATEGY_H

#include "Strategy.h"
#include <string>
#include <chrono>
#include <memory>

class NewsSentimentStrategy : public Strategy {
public:
    NewsSentimentStrategy(
        std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
        std::shared_ptr<DataHandler> data_handler,
        const std::string& symbol,
        double sentiment_threshold,
        int pause_minutes
    );

    // Implement pure virtual functions from base class
    void onMarket(const MarketEvent& event) override;
    void onTrade(const TradeEvent& event) override;
    void onOrderBook(const OrderBookEvent& event) override;
    void onFill(const FillEvent& event) override;
    void onNews(const NewsEvent& event);

private:
    bool is_trading_paused() const;
    
    double sentiment_threshold_;
    int pause_minutes_;

    std::chrono::system_clock::time_point pause_until_time_;
};

#endif // NEWS_SENTIMENT_STRATEGY_H