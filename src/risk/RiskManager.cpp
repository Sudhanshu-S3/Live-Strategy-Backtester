#include "../../include/risk/RishManager.h" 
#include "../../include/event/Event.h"
#include <cmath>
#include <iostream>
#include <memory> // For std::make_unique
#include <vector> // For performance calculation

// Helper function to convert OrderDirection enum to string for logging
std::string orderDirectionToString(OrderDirection dir) {
    switch (dir) {
        case OrderDirection::BUY: return "BUY";
        case OrderDirection::SELL: return "SELL";
        case OrderDirection::NONE: return "NONE"; // Added NONE
        default: return "UNKNOWN";
    }
}

RiskManager::RiskManager(EventQueue& events, Portfolio& portfolio, double risk_per_trade_pct, const RiskThresholds& thresholds) : 
    events_(events), 
    portfolio_(portfolio),
    risk_per_trade_pct_(risk_per_trade_pct),
    thresholds_(thresholds) {}

void RiskManager::onSignal(const SignalEvent& signal) {
    OrderDirection direction;
    if (signal.signal_type == "LONG") {
        direction = OrderDirection::BUY;
    } else if (signal.signal_type == "SHORT") {
        direction = OrderDirection::SELL;
    } else if (signal.signal_type == "EXIT") { // Handle EXIT signals for closing positions
        direction = OrderDirection::NONE; // Indicates closing, not opening a new direction
    } else { 
        return; // Ignore unknown signal types
    }

    // Get current total equity from the portfolio
    double equity = portfolio_.get_total_equity();
    if (equity <= 0) {
        std::cerr << "RISK MANAGER: Equity is zero or negative. Cannot place trades." << std::endl;
        return;
    }

    // Prevent opening a new position if one already exists in the same direction
    // For EXIT signals, this check is different; we allow closing.
    std::string current_pos_direction = portfolio_.getPositionDirection(signal.symbol);
    if (direction != OrderDirection::NONE) { // Only check for opening new positions
        if ((direction == OrderDirection::BUY && current_pos_direction == "LONG") || 
            (direction == OrderDirection::SELL && current_pos_direction == "SHORT")) {
            // std::cout << "RISK MANAGER: Already in a " << current_pos_direction << " position for " << signal.symbol << ". Ignoring signal to open new position." << std::endl;
            return;
        }
    }


    // --- Position Sizing Logic (for opening new positions) ---
    double quantity = 0.0;
    std::string order_type = "MKT"; // Default to market order

    if (direction == OrderDirection::NONE) { // EXIT signal
        // Logic for closing an existing position.
        // The quantity for an exit should be the current position quantity.
        quantity = std::abs(portfolio_.get_position(signal.symbol));
        if (quantity == 0) {
            // std::cout << "RISK MANAGER: No open position to exit for " << signal.symbol << ". Ignoring EXIT signal." << std::endl;
            return;
        }
        // Determine exit direction
        if (current_pos_direction == "LONG") {
            direction = OrderDirection::SELL;
        } else if (current_pos_direction == "SHORT") {
            direction = OrderDirection::BUY;
        }
    } else { // LONG or SHORT signal (opening new position)
        double last_price = portfolio_.get_last_price(signal.symbol);
        if (last_price <= 0) {
            std::cerr << "RISK MANAGER: Cannot get last price for " << signal.symbol << ". Ignoring signal." << std::endl;
            return;
        }

        double risk_amount = equity * risk_per_trade_pct_;
        // Assuming signal.stop_loss is provided and valid for risk calculation
        if (signal.stop_loss <= 0) { // Simple validation
             std::cerr << "RISK MANAGER: Invalid stop loss for " << signal.symbol << ". Ignoring signal." << std::endl;
             return;
        }
        
        double price_difference = std::abs(last_price - signal.stop_loss);

        if (price_difference < 1e-9) { 
            std::cerr << "RISK MANAGER: Stop loss is too close to the current price for " << signal.symbol << ". Ignoring signal." << std::endl;
            return; 
        }
        quantity = risk_amount / price_difference;
    }
    
    if (quantity <= 0) {
        std::cerr << "RISK MANAGER: Calculated quantity is zero or negative. Ignoring signal." << std::endl;
        return;
    }

    // --- Create and Push the Order Event ---
    OrderEvent order(
        signal.symbol,
        signal.timestamp,
        order_type, // OrderType is string in DataTypes.h
        orderDirectionToString(direction), // direction is string
        static_cast<long>(quantity) // Ensure quantity is integer for order size
    );

    std::cout << "RISK MANAGER: Approving trade for " << signal.symbol 
              << ". Direction=" << order.direction
              << ", Quantity=" << order.quantity 
              << ". Current Equity: " << equity << std::endl;

    events_.push(std::make_shared<OrderEvent>(order));
}

void RiskManager::monitorRealTimeRisk() {
    std::cout << "\nRISK MANAGER: Monitoring real-time risk..." << std::endl;

    // 1. Monitor Max Drawdown
    double current_max_drawdown = portfolio_.getMaxDrawdown();
    if (current_max_drawdown > thresholds_.max_drawdown_pct) {
        sendAlert("CRITICAL ALERT: Max Drawdown Exceeded! Current: " + 
                  std::to_string(current_max_drawdown * 100) + "% | Threshold: " + 
                  std::to_string(thresholds_.max_drawdown_pct * 100) + "%");
        // Implement automatic stop or strategy disable here if desired
    } else {
        std::cout << "Current Max Drawdown: " << std::fixed << std::setprecision(2) 
                  << current_max_drawdown * 100 << "% (Below threshold)" << std::endl;
    }

    // 2. Monitor VaR (using the latest equity curve from Portfolio)
    // VaR usually requires a series of returns. For real-time, we'd use recent returns.
    // The Performance class already takes the equity curve and calculates returns internally.
    Performance current_performance = portfolio_.getRealTimePerformance();
    double current_var_95 = current_performance.calculateVaR(0.95);

    if (current_var_95 > thresholds_.daily_var_95_pct) {
        sendAlert("HIGH ALERT: Daily VaR (95%) Exceeded! Current: " + 
                  std::to_string(current_var_95 * 100) + "% | Threshold: " + 
                  std::to_string(thresholds_.daily_var_95_pct * 100) + "%");
    } else {
        std::cout << "Current VaR (95%): " << std::fixed << std::setprecision(2) 
                  << current_var_95 * 100 << "% (Below threshold)" << std::endl;
    }

    // 3. Monitor Current Positions
    std::map<std::string, Position> current_positions = portfolio_.getCurrentPositions();
    if (!current_positions.empty()) {
        std::cout << "Current Open Positions:" << std::endl;
        for (const auto& pair : current_positions) {
            const Position& pos = pair.second;
            std::cout << "  " << pos.symbol << ": Quantity=" << pos.quantity 
                      << ", Avg Cost=" << std::fixed << std::setprecision(2) << pos.average_cost
                      << ", Market Value=" << std::fixed << std::setprecision(2) << pos.market_value 
                      << ", Direction=" << orderDirectionToString(pos.direction) << std::endl;
        }
    } else {
        std::cout << "No open positions." << std::endl;
    }
    std::cout << "--------------------------------------\n";
}

void RiskManager::sendAlert(const std::string& message) {
    std::cerr << "!!!!! RISK ALERT !!!!! " << message << std::endl;
    // In a real system, this would send an email, SMS, or push notification.
}