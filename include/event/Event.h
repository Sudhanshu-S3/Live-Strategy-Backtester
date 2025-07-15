#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <vector>

// Enums for clarity and type safety
enum class OrderDirection {
    BUY,
    SELL
};

enum class OrderType {
    MARKET,
    LIMIT
};

// Base struct for all events
struct Event {
    enum class EventType {
    MARKET,
    ORDER_BOOK,
    TRADE,
    SIGNAL,
    ORDER,
    FILL,
    NEWS // <--- ADDED
};
    virtual ~Event() = default;
    EventType type;
};

// Event for new market data
struct MarketEvent : public Event {
    std::string symbol;
    long long timestamp; // Using long long for timestamps
    double price; 

    MarketEvent(std::string symbol, long long timestamp, double price)
        : symbol(symbol), timestamp(timestamp), price(price) {
        this->type = Event::MARKET;
    }
};

// Event sent from a Strategy to the Portfolio
struct SignalEvent : public Event {
    std::string symbol;
    long long timestamp;
    OrderDirection direction;
    double strength;    // Confidence score (e.g., 1.0 for full size)
    double stop_loss;   // Price at which to place the stop-loss

    SignalEvent(std::string symbol, long long timestamp, OrderDirection direction, double stop_loss, double strength = 1.0)
        : symbol(symbol), timestamp(timestamp), direction(direction), stop_loss(stop_loss), strength(strength) {
        this->type = Event::SIGNAL;
    }
};

class NewsEvent : public Event {
public:
    std::string symbol;
    std::string timestamp;
    std::string headline;
    double sentiment_score;

    NewsEvent(std::string symbol, std::string timestamp, std::string headline, double sentiment_score)
        : symbol(std::move(symbol)), timestamp(std::move(timestamp)), 
          headline(std::move(headline)), sentiment_score(sentiment_score) {
        type = EventType::NEWS;
    }
};
// Event sent from Portfolio to the ExecutionHandler
struct OrderEvent : public Event {
    std::string symbol;
    long long timestamp;
    OrderType order_type;
    OrderDirection direction;
    double quantity;
    double stop_loss; // To pass SL info to execution

    OrderEvent(std::string symbol, long long timestamp, OrderType order_type, OrderDirection direction, double quantity, double stop_loss = 0.0)
        : symbol(symbol), timestamp(timestamp), order_type(order_type), direction(direction), quantity(quantity), stop_loss(stop_loss) {
        this->type = Event::ORDER;
    }
};

// Event sent from ExecutionHandler back to the Portfolio
struct FillEvent : public Event {
    std::string symbol;
    long long timestamp;
    OrderDirection direction;
    double quantity;
    double fill_price;
    double commission;

    FillEvent(long long timestamp, const std::string& symbol, OrderDirection direction, double quantity, double fill_price, double commission)
        : timestamp(timestamp), symbol(symbol), direction(direction), quantity(quantity), fill_price(fill_price), commission(commission) {
        this->type = Event::FILL;
    }
};

#endif