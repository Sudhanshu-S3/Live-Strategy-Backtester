#include "../../include/core/Portfolio.h"
#include <numeric> // For std::accumulate
#include <iostream>

Portfolio::Portfolio(double initial_capital, std::shared_ptr<DataHandler> data_handler)
    : initial_capital(initial_capital), 
      current_cash(initial_capital), 
      data_handler(data_handler) {}

void Portfolio::onFill(const FillEvent& fill) {
    if (fill.direction == "BUY") {
        double cost = fill.fill_price * fill.quantity;
        current_cash -= cost;
        current_cash -= fill.commission;

        auto it = holdings.find(fill.symbol);
        if (it != holdings.end()) {
            // Position exists, update it
            Position& position = it->second;
            double old_cost = position.average_cost * position.quantity;
            position.quantity += fill.quantity;
            position.average_cost = (old_cost + cost) / position.quantity;
        } else {
            // Position is new, create it
            holdings[fill.symbol] = Position{fill.symbol, fill.quantity, fill.fill_price};
        }
    } else if (fill.direction == "SELL") {
        double proceeds = fill.fill_price * fill.quantity;
        current_cash += proceeds;
        current_cash -= fill.commission;

        auto it = holdings.find(fill.symbol);
        if (it != holdings.end()) {
            it->second.quantity -= fill.quantity;
            // If we've sold all shares, remove the position
            if (it->second.quantity <= 0.0000001) { // Use a small epsilon for float comparison
                holdings.erase(it);
            }
        } else {
            // This would be an error in a real system (selling something you don't own)
            // but we'll log it for now.
            std::cerr << "Warning: Sold asset " << fill.symbol << " without a holding." << std::endl;
        }
    }
}

void Portfolio::updateTimeIndex() {
    double total_market_value = 0.0;

    // Iterate through all current holdings
    for (auto& pair : holdings) {
        Position& position = pair.second;
        auto latest_bar_opt = data_handler->getLatestBar(position.symbol);
        
        if (latest_bar_opt) {
            // Update the market value for this specific position
            position.market_value = latest_bar_opt->close * position.quantity;
            total_market_value += position.market_value;
        }
        // If there's no bar, we carry forward the last known market value
        // (This might happen if one asset's data feed pauses)
        else {
            total_market_value += position.market_value;
        }
    }

    // Total equity is the sum of all positions' market value plus free cash
    double total_equity = total_market_value + current_cash;
    equity_curve.push_back(total_equity);
}

const std::vector<double>& Portfolio::getEquityCurve() const {
    return equity_curve;
}

std::string Portfolio::getPositionDirection(const std::string& symbol) const {
    auto it = holdings.find(symbol);
    if (it != holdings.end() && it->second.quantity > 0) {
        // This is a simplified check. A real system would differentiate long/short.
        // For now, we assume any holding is a LONG holding. To support shorting,
        // the Position struct would need a 'direction' member.
        // Let's return "SELL" to close the position.
        return "SELL";
    }
    return "NONE"; // No position held.
}
