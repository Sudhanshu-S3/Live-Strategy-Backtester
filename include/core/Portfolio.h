#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <queue> // Required for event queue
#include "../data/DataTypes.h"
#include "../data/DataHandler.h"
#include "../core/Performance.h" // For performance metrics

// Represents our holding in a single asset.
struct Position {
    std::string symbol;
    double quantity = 0.0;
    double average_cost = 0.0; // The average price paid for the current holding
    double market_value = 0.0; // The current value of the holding
    OrderDirection direction = OrderDirection::NONE; // LONG or SHORT
};

// Represents a completed trade for performance analysis.
struct Trade {
    std::string symbol;
    OrderDirection direction;
    double quantity;
    double entry_price;
    double exit_price;
    double pnl;
    std::string entry_timestamp; // Consider using a more robust timestamp type
    std::string exit_timestamp;
};

class Portfolio {
public:
    // Constructor now requires the initial capital, a pointer to the DataHandler, and the event queue.
    Portfolio(std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue, double initial_capital, std::shared_ptr<DataHandler> data_handler);

    // --- Event Handlers ---

    // Processes a signal from the Strategy to generate an OrderEvent.
    void onSignal(const SignalEvent& signal);

    // Processes a fill from the ExecutionHandler to update positions and cash.
    void onFill(const FillEvent& fill);

    // Updates the market value of a specific holding based on a market event.
    void onMarket(const MarketEvent& market);
    
    // Updates the equity curve with the latest total portfolio value.
    void updateTimeIndex();

    // --- Performance & Reporting ---
    void generateReport();
    void writeResultsToCSV(const std::string& filename = "portfolio_performance.csv");
    void writeTradeLogToCSV(const std::string& filename = "trades_log.csv");

    // --- Getters ---
    double get_total_equity() const;
    double getInitialCapital() const { return initial_capital; } // Added getter
    const std::vector<std::pair<long long, double>>& getEquityCurve() const; // Renamed for clarity
    std::string getPositionDirection(const std::string& symbol) const;
    double get_position(const std::string& symbol) const;
    double get_last_price(const std::string& symbol) const;
    
    // New: Real-Time Monitoring
    double getRealTimePnL() const;
    std::map<std::string, Position> getCurrentPositions() const;
    // New: Calculate current performance metrics based on real-time data
    Performance getRealTimePerformance() const;
Portfolio(std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue, double initial_capital, std::shared_ptr<DataHandler> data_handler);

    void onFill(const FillEvent& fill);
    void onMarket(const MarketEvent& market);
    void updateTimeIndex();

    double get_total_equity() const;
    double get_position(const std::string& symbol) const;
    std::string getPositionDirection(const std::string& symbol) const;
    double get_last_price(const std::string& symbol) const;
    const std::vector<std::pair<long long, double>>& getEquityCurve() const;
    double getInitialCapital() const { return initial_capital; }
    Performance getRealTimePerformance() const;
    double getRealTimePnL() const;
    std::map<std::string, Position> getCurrentPositions() const;

    void generateReport();
    void writeResultsToCSV(const std::string& filename);
    void writeTradeLogToCSV(const std::string& filename);

private:
    double initial_capital;
    double current_cash;
    double total_equity;
    double peak_equity;
    double max_drawdown;

    std::map<std::string, Position> holdings;
    std::vector<std::pair<long long, double>> equity_curve;
    std::vector<ClosedTrade> trade_log;

    std::shared_ptr<DataHandler> data_handler;
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue;

    // STAGE 4: Added private helper for detailed trade analysis
    void generateTradeLevelReport() const;
    std::shared_ptr<DataHandler> data_handler;
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue; // To send OrderEvents
    double initial_capital;
    double current_cash;

    // A map to store our position in each symbol. Key: symbol string.
    std::map<std::string, Position> holdings;
    
    // --- Performance Tracking ---
    double total_equity; // Includes cash + market value of all positions
    double peak_equity;  // For drawdown calculation
    double max_drawdown; // For drawdown calculation

    // A timeseries list of the total portfolio value at each bar.
    std::vector<std::pair<long long, double>> equity_curve; // Stores timestamp and equity
    std::vector<Trade> trade_log; // Stores details of each completed trade
};

#endif