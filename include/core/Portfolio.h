#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../data/DataTypes.h"
#include "../data/DataHandler.h"

// Represents our holding in a single asset.
struct Position {
    std::string symbol;
    double quantity = 0.0;
    double average_cost = 0.0; // The average price paid for the current holding
    double market_value = 0.0; // The current value of the holding
};

class Portfolio {
public:
    // Constructor now requires the initial capital and a pointer to the DataHandler.
    Portfolio(double initial_capital, std::shared_ptr<DataHandler> data_handler);

    // Updates portfolio state based on a fill event from the execution handler.
    void onFill(const FillEvent& fill);
    
    // Updates the market value of all holdings and records the new total equity.
    // This should be called once per heartbeat of the backtest.
    void updateTimeIndex();
    std::string getPositionDirection(const std::string& symbol) const;

    // Getter for the equity curve
    const std::vector<double>& getEquityCurve() const;

    // Getter for current cash
    double getCurrentCash() const { return current_cash; }

private:
    std::shared_ptr<DataHandler> data_handler;
    double initial_capital;
    double current_cash;

    // A map to store our position in each symbol. Key: symbol string.
    std::map<std::string, Position> holdings;
    
    // A timeseries list of the total portfolio value at each bar.
    std::vector<double> equity_curve;
};

#endif // PORTFOLIO_H