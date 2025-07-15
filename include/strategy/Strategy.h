#ifndef STRATEGY_H
#define STRATEGY_H

#include "../event/Event.h"
#include "../data/DataHandler.h"
#include "../data/DataTypes.h" // For MarketRegime if passed directly

#include <memory>
#include <queue>
#include <string>

// Forward declarations to avoid circular includes
class EventQueue;
class DataHandler; // Assuming DataHandler.h is included where this is used

// Base class for all trading strategies.
class Strategy {
public:
    Strategy(std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
             std::shared_ptr<DataHandler> data_handler);
    virtual ~Strategy() = default;

    // --- Event Handlers (pure virtual, must be implemented by derived strategies) ---
    virtual void onMarket(const MarketEvent& event) = 0;
    virtual void onTrade(const TradeEvent& event) = 0;
    virtual void onOrderBook(const OrderBookEvent& event) = 0;
    virtual void onFill(const FillEvent& event) = 0; // New: Handle fill events in strategy

    // New: Handle market regime changes (optional for derived classes)
    virtual void onMarketRegime(MarketRegime new_regime) {
        // Default implementation does nothing. Derived strategies can override.
    }

protected:
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue_;
    std::shared_ptr<DataHandler> data_handler_;
};

#endif // STRATEGY_H