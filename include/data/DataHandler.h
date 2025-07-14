#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include <optional>
#include <string>
#include <vector>
#include <queue>
#include <memory>
#include "DataTypes.h"

using namespace std;

// The DataHandler is responsible for managing and providing market data.
// In a multi-asset system, its key job is to update the main event queue
// with new MarketEvents in correct chronological order.
class DataHandler {
public:
    virtual ~DataHandler() = default;

    // Pushes the next available bar data as a MarketEvent onto the queue.
    // This is the new "heartbeat" of the backtest.
    virtual void updateBars(queue<shared_ptr<Event>>& event_queue) = 0;
    
    // Checks if there is no more data to process.
    virtual bool isFinished() const = 0;

    // Gets the most recently loaded bar for a specific symbol.
    // The Strategy and Portfolio will use this to get detailed OHLCV data.
    virtual optional<Bar> getLatestBar(const string& symbol) const = 0;

    // Gets the last known value for a symbol (e.g., "price").
    // This provides a unified way to get the latest price, whether
    virtual double getLatestBarValue(const std::string& symbol, const std::string& val_type) = 0;

    // Gets the n most recently loaded bars for a specific symbol.
    virtual std::vector<Bar> getLatestBars(const std::string& symbol, int n = 1) = 0;
};

#endif