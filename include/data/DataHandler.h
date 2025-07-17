#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include <optional>
#include <string>
#include <vector>
#include <queue>
#include <memory>
#include <functional> // <-- Add this line
#include "DataTypes.h"

using namespace std;

// The DataHandler is responsible for managing and providing market data.
// In a multi-asset system, its key job is to update the main event queue
// with new MarketEvents in correct chronological order.
class DataHandler {
public:
    virtual ~DataHandler() = default;

    // Main loop processing function
    virtual void updateBars() = 0;

    // Returns true when all data has been processed.
    virtual bool isFinished() const = 0;

    // Gets the latest loaded bar for a specific symbol.
    virtual std::optional<Bar> getLatestBar(const std::string& symbol) const = 0;

    // Gets a specific value from the latest bar for a symbol.
    virtual double getLatestBarValue(const std::string& symbol, const std::string& val_type) = 0;

    // Gets the n most recently loaded bars for a specific symbol.
    virtual std::vector<Bar> getLatestBars(const std::string& symbol, int n = 1) = 0;

    // Gets the latest order book for a specific symbol.
    virtual std::optional<OrderBook> getLatestOrderBook(const std::string& symbol) const = 0;

    // Gets the list of symbols the data handler is managing.
    virtual const std::vector<std::string>& getSymbols() const = 0;

    // For event-driven systems, allows external components to know when new data is ready.
    virtual void notifyOnNewData(std::function<void()> callback) = 0;

protected:
    std::function<void()> on_new_data_;
};

#endif // DATAHANDLER_H