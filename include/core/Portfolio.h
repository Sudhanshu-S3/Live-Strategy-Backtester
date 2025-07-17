#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Performance.h"
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

class Portfolio {
public:
    Portfolio(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue, 
              double initial_capital, 
              std::shared_ptr<DataHandler> data_handler);

    double get_cash() const;
    double get_max_drawdown() const;
    // Assuming Bar is a struct/class you have defined
    std::vector<Bar> get_latest_bars(const std::string& symbol, int lookback) const;

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
    double getInitialCapital() const { return initial_capital_; }
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
    double initial_capital_;
    double current_cash_;
    double total_equity_;
    double peak_equity_;
    double max_drawdown_;

    std::map<std::string, Position> holdings_;
    std::vector<std::tuple<long long, double, MarketState>> equity_curve_;
    std::vector<Trade> trade_log_; 
    std::map<std::string, std::vector<Trade>> strategy_trade_log_;

    std::shared_ptr<DataHandler> data_handler_;
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue_;
    MarketState current_market_state_;

    void generateTradeLevelReport() const;
};

#endif