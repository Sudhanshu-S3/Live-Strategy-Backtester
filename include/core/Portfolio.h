#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <queue> // Required for event queue
#include "../data/DataTypes.h"
#include "../data/DataHandler.h"

// Represents our holding in a single asset.
struct Position {
    std::string symbol;
    double quantity = 0.0;
    double average_cost = 0.0; // The average price paid for the current holding
    double market_value = 0.0; // The current value of the holding
};

// Represents a completed trade for performance analysis.
struct Trade {
    std::string symbol;
    OrderDirection direction;
    double quantity;
    double entry_price;
    double exit_price;
    double pnl;
    long long entry_timestamp; // Consider using a more robust timestamp type
    long long exit_timestamp;
};

class Portfolio {
public:
    // Constructor now requires the initial capital, a pointer to the DataHandler, and the event queue.
    Portfolio(double initial_capital, std::shared_ptr<DataHandler> data_handler, std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue);

    // --- Event Handlers ---

    // Processes a signal from the Strategy to generate an OrderEvent.
    void onSignal(const SignalEvent& signal);

    // Updates portfolio state based on a fill event from the execution handler.
    void onFill(const FillEvent& fill);

    // Updates the market value of a specific holding based on a market event.
    void onMarket(const MarketEvent& market);
    
    // Updates the equity curve with the latest total portfolio value.
    void updateTimeIndex();

    // --- Performance & Reporting ---
    void generateReport();
    void writeResultsToCSV();

    // --- Getters ---
    double get_total_equity() const;
    const std::vector<std::pair<long long, double>>& getEquityCurve() const;
    std::string getPositionDirection(const std::string& symbol) const;
    double get_position(const std::string& symbol) const;
    double get_last_price(const std::string& symbol) const;
    
private:
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
    std::vector<std::pair<long long, double>> equity_curve;
    
    // A log of all completed trades for performance analysis.
    std::vector<Trade> trade_log;
};

#endif // PORTFOLIO_H