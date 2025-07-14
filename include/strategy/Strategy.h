#ifndef STRATEGY_H
#define STRATEGY_H

#include "DataTypes.h"
#include <optional>

// Abstract base class for all trading strategies.
class Strategy {
public:
    virtual ~Strategy() = default;

    // It takes the latest bar and returns a SignalEvent.
    virtual SignalEvent generateSignals(const Bar& bar) = 0;
};

#endif