#ifndef DATATYPES_H
#define DATATYPES_H

#include <string>
#include <vector>
#include <map>

// Represents a single bar of data (Open, High, Low, Close, Volume) for a symbol.
struct Bar {
    std::string symbol = "";
    std::string timestamp = "";
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    long long volume = 0;
};

// Represents a single executed trade from the exchange.
struct Trade {
    std::string symbol = "";
    long long timestamp = 0; // Unix timestamp in milliseconds
    double price = 0.0;
    double quantity = 0.0;
    std::string aggressor_side;
};

// Represents a single level (price and quantity) in the order book.
struct OrderBookLevel {
    double price = 0.0;
    double quantity = 0.0;
};

// Represents a full snapshot of the order book at a point in time.
struct OrderBook {
    std::string symbol = "";
    long long timestamp = 0; // Unix timestamp
    std::vector<std::pair<double, double>> bids;
    std::vector<std::pair<double, double>> asks;
};

// --- Market State ---
enum class VolatilityLevel {
    LOW,
    NORMAL,
    HIGH
};

enum class TrendDirection {
    SIDEWAYS,
    TRENDING_UP,
    TRENDING_DOWN
};

struct MarketState {
    VolatilityLevel volatility = VolatilityLevel::NORMAL;
    TrendDirection trend = TrendDirection::SIDEWAYS;
    double volatility_value = 0.0;
};

#endif // DATATYPES_H