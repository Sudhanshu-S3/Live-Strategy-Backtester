#include "../../include/core/Portfolio.h"
#include "../../include/data/DataTypes.h" // Make sure this is included for Event types
#include <iostream>
#include <numeric>

Portfolio::Portfolio(double initial_capital, std::shared_ptr<DataHandler> data_handler, std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue)
    : initial_capital(initial_capital), 
      current_cash(initial_capital), 
      data_handler(data_handler),
      event_queue(event_queue) {}

void Portfolio::onFill(const FillEvent& fill) {
    if (fill.direction == "BUY") {
        if (holdings.find(fill.symbol) == holdings.end()) {
            // New position
            holdings[fill.symbol] = Position{fill.symbol, static_cast<double>(fill.quantity), fill.fill_price};
        } else {
            // Increase existing position
            Position& pos = holdings[fill.symbol];
            double old_cost = pos.average_cost * pos.quantity;
            pos.quantity += fill.quantity;
            pos.average_cost = (old_cost + fill.fill_price * fill.quantity) / pos.quantity;
        }
        current_cash -= fill.fill_price * fill.quantity;
        current_cash -= fill.commission;
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

// Processes a signal from the Strategy to generate an OrderEvent.
void Portfolio::onSignal(const SignalEvent& signal) {
    std::string order_direction = "";
    int order_quantity = 100; // Example: default order size, you can make this more sophisticated.

    if (signal.signal_type == "LONG") {
        order_direction = "BUY";
    } else if (signal.signal_type == "SHORT") {
        order_direction = "SELL";
    } else if (signal.signal_type == "EXIT") {
        auto it = holdings.find(signal.symbol);
        if (it != holdings.end() && it->second.quantity > 0) {
            order_direction = "SELL"; // Assuming we are closing a long position
            order_quantity = static_cast<int>(it->second.quantity);
        } else {
            return; // No position to exit
        }
    }

    if (!order_direction.empty() && order_quantity > 0) {
        // Create an OrderEvent and push it to the event queue.
        auto order = std::make_shared<OrderEvent>(signal.symbol, "MARKET", order_quantity, order_direction);
        event_queue->push(order);
    }
}

// Updates the market value of a specific holding based on a market event.
void Portfolio::onMarket(const MarketEvent& market) {
    auto it = holdings.find(market.symbol);
    if (it != holdings.end()) {
        // Use the latest price from the DataHandler to update the market value
        double latest_price = data_handler->getLatestBarValue(market.symbol, "close");
        if (latest_price > 0) { // Ensure we have a valid price
             it->second.market_value = it->second.quantity * latest_price;
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
