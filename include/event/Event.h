#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <vector>
#include <chrono>
#include "../data/DataTypes.h"

// enum class OrderDirection { BUY, SELL, NONE }; // REMOVED: Moved to DataTypes.h
// enum class OrderType { MARKET, LIMIT }; // REMOVED: Already defined in DataTypes.h

enum class EventType {
    MARKET,
    SIGNAL,
    ORDER,
    FILL,
    TRADE,
    ORDER_BOOK,
    MARKET_REGIME_CHANGED,
    DATA_SOURCE_STATUS,
    NEWS,  // Add this line
    UNKNOWN // Add UNKNOWN type
};

struct Event {
    EventType type;
    long long timestamp_received = 0; // Add this field
    
    // Default constructor
    Event() : type(EventType::UNKNOWN) {}
    
    virtual ~Event() = default;
};

struct MarketEvent : public Event {
    std::string symbol;
    long long timestamp;
    double price;
    MarketEvent(std::string symbol, long long ts, double p) 
        : symbol(std::move(symbol)), timestamp(ts), price(p) { type = EventType::MARKET; }
};

struct TradeEvent : public Event {
    std::string symbol;
    long long timestamp;
    double price;
    double quantity;
    std::string aggressor_side;
    TradeEvent(std::string s, long long ts, double p, double q, std::string side)
        : symbol(std::move(s)), timestamp(ts), price(p), quantity(q), aggressor_side(std::move(side)) { type = EventType::TRADE; }
};

// Event for new market data
#include "OrderBookEvent.h"

// Event sent from a Strategy to the Portfolio
struct SignalEvent : public Event {
    std::string strategy_name;
    std::string symbol;
    long long timestamp;
    OrderDirection direction;
    double strength;    // Confidence score (e.g., 1.0 for full size)
    double stop_loss;   // Price at which to place the stop-loss

    SignalEvent(std::string strategy_name, std::string symbol, long long timestamp, OrderDirection direction, double stop_loss, double strength = 1.0)
        : strategy_name(std::move(strategy_name)), symbol(std::move(symbol)), timestamp(timestamp), direction(direction), stop_loss(stop_loss), strength(strength) {
        this->type = EventType::SIGNAL;
    }
};

struct NewsEvent : public Event {
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
    OrderDirection direction;
    double quantity;
    OrderType order_type;
    std::string strategy_name;
    long order_id; // <-- ADD THIS LINE

    OrderEvent(std::string symbol, long long timestamp, OrderDirection direction, double quantity, OrderType order_type, std::string strategy_name)
        : symbol(std::move(symbol)), timestamp(timestamp), direction(direction), quantity(quantity), order_type(order_type), strategy_name(std::move(strategy_name)) {
        this->type = EventType::ORDER;
        static long id_counter = 0;
        this->order_id = ++id_counter;
    }
};

// Event sent from ExecutionHandler back to the Portfolio
struct FillEvent : public Event {
    std::string strategy_name;
    std::string symbol;
    long long timestamp;
    OrderDirection direction;
    double quantity;
    double fill_price;
    double commission;

    FillEvent(long long timestamp, const std::string& symbol, const std::string& strategy_name, OrderDirection direction, double quantity, double fill_price, double commission)
        : timestamp(timestamp), symbol(symbol), strategy_name(strategy_name), direction(direction), quantity(quantity), fill_price(fill_price), commission(commission) {
        this->type = EventType::FILL;
    }
};

// --- New System-Level Events ---

enum class DataSourceStatus { CONNECTED, DISCONNECTED, RECONNECTING, FALLBACK_ACTIVE };

struct DataSourceStatusEvent : public Event {
    DataSourceStatus status;
    std::string message;

    DataSourceStatusEvent(DataSourceStatus status, std::string message)
        : status(status), message(std::move(message)) {
        type = EventType::DATA_SOURCE_STATUS;
    }
};

struct MarketRegimeChangedEvent : public Event {
    MarketState new_state;
    MarketRegimeChangedEvent(const MarketState& state) : new_state(state) {
        type = EventType::MARKET_REGIME_CHANGED;
    }
};

// STAGE 7: Event for failed/rejected orders
struct OrderFailureEvent : public Event {
    long long timestamp;
    std::string symbol;
    long order_id;
    std::string reason;

    OrderFailureEvent(long long ts, std::string sym, long id, std::string r)
        : timestamp(ts), symbol(std::move(sym)), order_id(id), reason(std::move(r)) {
        this->type = EventType::ORDER; // Or a new type if needed
    }
};

#endif