#ifndef STRATEGY_H
#define STRATEGY_H

#include "../event/Event.h"
#include "../data/DataHandler.h"
#include "../data/DataTypes.h"
#include "../event/ThreadSafeQueue.h"

#include <memory>
#include <string>
#include <vector>

// Forward declarations to avoid circular includes
class DataHandler;

// Base class for all trading strategies.
class Strategy {
public:
    Strategy(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue, 
             std::shared_ptr<DataHandler> data_handler,
             const std::string& name,
             const std::string& symbol)
        : event_queue_(event_queue), data_handler_(data_handler), name(name), symbol(symbol) {}
    
    virtual ~Strategy() = default;

    // --- Event Handlers (pure virtual, must be implemented by derived strategies) ---
    virtual void onMarket(const MarketEvent& event) = 0;
    virtual void onTrade(const TradeEvent& event) = 0;
    virtual void onOrderBook(const OrderBookEvent& event) = 0;
    virtual void onFill(const FillEvent& event) = 0;
    virtual void onMarketRegimeChanged(const MarketRegimeChangedEvent& event) {
        market_state_ = event.new_state;
    }

    // --- Getters & Setters ---
    std::string getName() const { return name; }
    std::string getSymbol() const { return symbol; }
    bool isPaused() const { return paused_; }
    void pause() { paused_ = true; }
    void resume() { paused_ = false; }

protected:
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue_;
    std::shared_ptr<DataHandler> data_handler_;
    std::string name;
    std::string symbol;
    bool paused_ = false;
    MarketState market_state_;
};

#endif // STRATEGY_H