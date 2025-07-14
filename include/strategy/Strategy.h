#ifndef STRATEGY_H
#define STRATEGY_H

#include <queue>
#include <memory>
#include "../data/DataTypes.h"

// The Strategy class is responsible for generating trading signals
// based on incoming market data.
class Strategy {
public:
    virtual ~Strategy() = default;

    // Reacts to a market event and may push SignalEvents to the queue.
    // This is now the core method for all strategies.
    virtual void generateSignals(
        const MarketEvent& event, 
        std::queue<std::shared_ptr<Event>>& event_queue
    ) = 0;
};

#endif // STRATEGY_H