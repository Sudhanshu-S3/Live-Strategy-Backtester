#include "../../include/risk/RishManager.h" // Corrected typo from RishManager.h
#include "../../include/event/Event.h"
#include <cmath>
#include <iostream>
#include <memory> // For std::make_unique

// Helper function to convert OrderDirection enum to string for logging
std::string orderDirectionToString(OrderDirection dir) {
    switch (dir) {
        case OrderDirection::BUY: return "BUY";
        case OrderDirection::SELL: return "SELL";
        default: return "UNKNOWN";
    }
}

RiskManager::RiskManager(EventQueue& events, Portfolio& portfolio, double risk_per_trade_pct) : 
    events_(events), 
    portfolio_(portfolio),
    risk_per_trade_pct_(risk_per_trade_pct) {}

void RiskManager::onSignal(const SignalEvent& signal) {
    OrderDirection direction;
    if (signal.signal_type == "LONG") {
        direction = OrderDirection::BUY;
    } else if (signal.signal_type == "SHORT") {
        direction = OrderDirection::SELL;
    } else { // Includes "EXIT" and any other type
        // Current risk manager logic doesn't handle closing positions,
        // so we'll ignore EXIT signals for now.
        return;
    }

    // Get current total equity from the portfolio
    double equity = portfolio_.get_total_equity();
    if (equity <= 0) {
        std::cerr << "RISK MANAGER: Cannot trade with zero or negative equity." << std::endl;
        return;
    }

    // Get the last known price for the symbol from the portfolio
    double last_price = portfolio_.get_last_price(signal.symbol);
    if (last_price <= 0) {
        std::cerr << "RISK MANAGER: Could not get a valid last price for " << signal.symbol << ". Ignoring signal." << std::endl;
        return;
    }

    // --- Check for existing positions and conflicting signals ---
    double current_position = portfolio_.get_position(signal.symbol);
    if ((direction == OrderDirection::BUY && current_position > 0) || (direction == OrderDirection::SELL && current_position < 0)) {
        return;
    }

    // --- Position Sizing Logic ---
    double risk_amount = equity * risk_per_trade_pct_;
    double price_difference = std::abs(last_price - signal.stop_loss);

    if (price_difference < 1e-9) { 
        std::cerr << "RISK MANAGER: Stop loss is too close to the current price for " << signal.symbol << ". Ignoring signal." << std::endl;
        return; 
    }

    double quantity = risk_amount / price_difference;

    if (quantity <= 0) {
        std::cerr << "RISK MANAGER: Calculated quantity is zero or negative. Ignoring signal." << std::endl;
        return;
    }

    // --- Create and Push the Order Event ---
    OrderEvent order(
        signal.symbol,
        signal.timestamp,
        "MKT", // OrderType is string in DataTypes.h
        direction == OrderDirection::BUY ? "BUY" : "SELL", // direction is string
        static_cast<int>(quantity)
    );

    std::cout << "RISK MANAGER: Approving trade for " << signal.symbol 
              << ". Direction=" << orderDirectionToString(direction)
              << ", Quantity=" << quantity 
              << ", Entry Price=" << last_price
              << ", Stop Loss=" << signal.stop_loss
              << std::endl;

    events_.push(std::make_unique<OrderEvent>(order));
}