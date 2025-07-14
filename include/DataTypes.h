#ifndef DATATYPES_H
#define DATATYPES_H

#include <cstdint>
#include <string>

using namespace std;

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


enum class OrderDirection {
    BUY,
    SELL
};


struct OrderEvent {
    uint64_t timestamp;
    string symbol;
    OrderDirection direction;
    double quantity;
};


struct FillEvent {
    uint64_t timestamp;
    string symbol;
    OrderDirection direction;
    double quantity;
    double fill_price;
    double commission;
};

#endif 