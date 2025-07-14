#ifndef DATATYPES_H
#define DATATYPES_H

#include <string>
#include <vector>

// --- Core Event Base Struct ---
// Base struct for all events in the system.
struct Event {
    enum EventType {
        MARKET,
        SIGNAL,
        ORDER,
        FILL,
        TRADE,      // New event type for a single trade
        ORDERBOOK   // New event type for an order book update
    };

    virtual ~Event() = default;
    EventType type;
};


// --- Original Data Structures (for OHLCV bars) ---

// Represents a single bar of data (Open, High, Low, Close, Volume) for a symbol.
struct Bar {
    std::string symbol = "";
    std::string timestamp = "";
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    long volume = 0;
};

// Event for when new market data (a Bar) is available.
struct MarketEvent : public Event {
    MarketEvent(std::string symbol, std::string timestamp) : symbol(symbol), timestamp(timestamp) {
        type = MARKET;
    }
    std::string symbol;
    std::string timestamp;
};


// --- New High-Frequency Data Structures ---

// Represents a single executed trade from the exchange.
// Corresponds to the data from our first Python script.
struct Trade {
    std::string symbol = "";
    long timestamp = 0; // Unix timestamp in milliseconds
    double price = 0.0;
    double quantity = 0.0;
    bool is_buyer_maker = false;
};

// Represents a single level (price and quantity) in the order book.
struct OrderBookLevel {
    double price = 0.0;
    double quantity = 0.0;
};

// Represents a full snapshot of the order book at a point in time.
// Corresponds to the data from our WebSocket recorder script.
struct OrderBook {
    std::string symbol = "";
    long timestamp = 0; // Unix timestamp
    std::vector<OrderBookLevel> bids;
    std::vector<OrderBookLevel> asks;
};


// --- New High-Frequency Event Types ---

// Event for when a new trade occurs in the market.
struct TradeEvent : public Event {
    TradeEvent(Trade trade_data) : trade(trade_data) {
        type = TRADE;
    }
    Trade trade;
};

// Event for when a new order book snapshot is available.
struct OrderBookEvent : public Event {
    OrderBookEvent(OrderBook book_data) : book(book_data) {
        type = ORDERBOOK;
    }
    OrderBook book;
};


// --- Original Strategy & Execution Structures ---

// Event sent from a Strategy to the Portfolio.
struct SignalEvent : public Event {
    SignalEvent(const std::string& symbol, const std::string& timestamp, const std::string& signal_type, double strength = 1.0)
        : symbol(symbol), timestamp(timestamp), signal_type(signal_type), strength(strength) {
        type = SIGNAL;
    }
    std::string symbol;
    std::string timestamp;
    std::string signal_type; // "LONG", "SHORT", "EXIT"
    double strength;
};

// Event sent from Portfolio to the ExecutionHandler.
struct OrderEvent : public Event {
    OrderEvent(const std::string& symbol, const std::string& order_type, int quantity, const std::string& direction)
        : symbol(symbol), order_type(order_type), quantity(quantity), direction(direction) {
        type = ORDER;
    }
    std::string symbol;
    std::string order_type; // "MKT" or "LMT"
    int quantity = 0;
    std::string direction; // "BUY" or "SELL"
};

// Event sent from ExecutionHandler back to the Portfolio.
struct FillEvent : public Event {
    FillEvent(const std::string& timestamp, const std::string& symbol, const std::string& exchange,
              int quantity, const std::string& direction, double fill_price, double commission)
        : timestamp(timestamp), symbol(symbol), exchange(exchange), quantity(quantity),
          direction(direction), fill_price(fill_price), commission(commission) {
        type = FILL;
    }
    std::string timestamp;
    std::string symbol;
    std::string exchange;
    int quantity = 0;
    std::string direction;
    double fill_price = 0.0;
    double commission = 0.0;
};

#endif // DATATYPES_H