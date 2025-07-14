#ifndef STRATEGY_H
#define STRATEGY_H

#include <queue>
#include <memory>
#include "../data/DataTypes.h"

// The Strategy class is responsible for generating trading signals
// based on incoming market data.
class Strategy {
public:
    // Constructor now takes the event queue.
    Strategy(std::shared_ptr<std::queue<std::shared_ptr<Event>>> events_queue) 
        : events_queue_(events_queue) {}
    virtual ~Strategy() = default;

    // --- Event Handlers for different data types ---
    // Derived strategies should override the handlers for the events they care about.

    // Reacts to a top-of-book or bar update event.
    virtual void onMarket(const MarketEvent& event) {}

    // Reacts to a new trade in the market.
    virtual void onTrade(const TradeEvent& event) {}

    // Reacts to a change in the order book.
    virtual void onOrderBook(const OrderBookEvent& event) {}

    // Reacts to a confirmation that one of its own orders was filled.
    virtual void onFill(const FillEvent& event) {}

protected:
    // The event queue for the strategy to push SignalEvents to.
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> events_queue_;
};

#endif // STRATEGY_H