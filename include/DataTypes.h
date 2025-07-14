#ifndef DATATYPES_H
#define DATATYPES_H

#include <cstdint>
#include <string>

using namespace std;

// --- Bar and Signal Types (from Day 1) ---

struct Bar {
    uint64_t timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume;
};

enum class SignalType {
    LONG,
    SHORT,
    EXIT,
    DO_NOTHING
};

struct SignalEvent {
    uint64_t timestamp;
    string symbol;
    SignalType type;
};

// --- New for Day 2: Order and Fill Types ---

// Defines the direction of an order.
enum class OrderDirection {
    BUY,
    SELL
};

// Represents an instruction to the execution system to place a trade.
struct OrderEvent {
    uint64_t timestamp;
    string symbol;
    OrderDirection direction;
    double quantity;
    // In a real system, you'd also have order_type (MARKET, LIMIT), etc.
};

// Represents a filled order, confirming a trade has occurred.
struct FillEvent {
    uint64_t timestamp;
    string symbol;
    OrderDirection direction;
    double quantity;
    double fill_price;
    double commission;
};

#endif // DATATYPES_H