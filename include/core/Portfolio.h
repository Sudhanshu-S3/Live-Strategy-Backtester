#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../event/ThreadSafeQueue.h"
#include "../data/DataTypes.h"
#include "../data/DataHandler.h"
#include "../core/Performance.h"
#include "../strategy/MarketRegimeDetector.h"

// Represents our holding in a single asset.
struct Position {
    std::string symbol;
    double quantity = 0.0;
    double average_cost = 0.0;
    double market_value = 0.0;
    OrderDirection direction = OrderDirection::NONE;
};

// Represents a completed trade for performance analysis.
struct Trade {
    std::string symbol;
    OrderDirection direction;
    double quantity;
    double entry_price;
    double exit_price;
    double pnl;
    std::string entry_timestamp;
    std::string exit_timestamp;
    MarketState market_state_at_entry;
};

class Portfolio {
public:
    Portfolio(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue, 
              double initial_capital, 
              std::shared_ptr<DataHandler> data_handler);

    // --- Event Handlers ---
    void onSignal(const SignalEvent& signal);
    void onFill(const FillEvent& fill);
    void onMarket(const MarketEvent& market);
    void onMarketRegimeChanged(const MarketRegimeChangedEvent& event);
    void updateTimeIndex();

    // --- Performance & Reporting ---
    void generateReport();
    void writeResultsToCSV(const std::string& filename = "portfolio_performance.csv");
    void writeTradeLogToCSV(const std::string& filename = "trades_log.csv");

    // --- Getters ---
    double get_total_equity() const;
    double getInitialCapital() const { return initial_capital; }
    const std::vector<std::tuple<long long, double, MarketState>>& getEquityCurve() const;
    std::string getPositionDirection(const std::string& symbol) const;
    double get_position(const std::string& symbol) const;
    double get_last_price(const std::string& symbol) const;
    const std::vector<Trade>& getTradeLog() const { return trade_log_; }
    const std::map<std::string, std::vector<Trade>>& getStrategyTradeLog() const { return strategy_trade_log_; }
    
    // New: Real-Time Monitoring
    double getRealTimePnL() const;
    std::map<std::string, Position> getCurrentPositions() const;
    Performance getRealTimePerformance() const;

private:
    double initial_capital;
    double current_cash;
    double total_equity;
    double peak_equity;
    double max_drawdown;

    std::map<std::string, Position> holdings;
    std::vector<std::tuple<long long, double, MarketState>> equity_curve;
    std::vector<Trade> trade_log; 
    std::map<std::string, std::vector<Trade>> strategy_trade_log_;

    std::shared_ptr<DataHandler> data_handler;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue;
    MarketState current_market_state_;

    void generateTradeLevelReport() const;
};

#endif