#ifndef NEWS_SENTIMENT_STRATEGY_H
#define NEWS_SENTIMENT_STRATEGY_H

#include "Strategy.h"
#include <string>
#include <chrono>

class NewsSentimentStrategy : public Strategy {
public:
    NewsSentimentStrategy(EventQueue& q, double sentiment_threshold, int pause_minutes);

    void calculate_signals(const Event& event) override;

private:
    void on_news(const NewsEvent& event);
    void on_market_data(const OrderBookEvent& event);
    bool is_trading_paused() const;
    
    double sentiment_threshold;
    int pause_minutes;

    std::chrono::system_clock::time_point pause_until_time;
};

#endif // NEWS_SENTIMENT_STRATEGY_H