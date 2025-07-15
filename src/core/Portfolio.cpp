#include "../../include/core/Portfolio.h"
#include "../../include/event/Event.h" 
#include <iostream>
#include <numeric>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

Portfolio::Portfolio(double initial_capital, std::shared_ptr<DataHandler> data_handler, std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue)
    : initial_capital(initial_capital), 
      current_cash(initial_capital),
      total_equity(initial_capital),
      peak_equity(initial_capital),
      max_drawdown(0.0),
      data_handler(data_handler),
      event_queue(event_queue) {}

void Portfolio::onFill(const FillEvent& fill) {
    auto it = holdings.find(fill.symbol);
    double previous_quantity = (it == holdings.end()) ? 0.0 : it->second.quantity;

    if (fill.direction == OrderDirection::BUY) {
        current_cash -= (fill.fill_price * fill.quantity) + fill.commission;
        
        if (it == holdings.end()) {
            // New position
            holdings[fill.symbol] = Position{fill.symbol, static_cast<double>(fill.quantity), fill.fill_price, 0.0};
        } else {
            // Increase existing position
            Position& pos = it->second;
            double old_cost = pos.average_cost * pos.quantity;
            pos.quantity += fill.quantity;
            pos.average_cost = (old_cost + fill.fill_price * fill.quantity) / pos.quantity;
        }
    } else if (fill.direction == OrderDirection::SELL) {
        if (it == holdings.end() || it->second.quantity < fill.quantity) {
             std::cerr << "Warning: Attempting to sell more of " << fill.symbol << " than is held." << std::endl;
             return; // Or handle error appropriately
        }

        double proceeds = fill.fill_price * fill.quantity;
        current_cash += proceeds - fill.commission;

        double pnl = (fill.fill_price - it->second.average_cost) * fill.quantity;
        
        it->second.quantity -= fill.quantity;

        // Check if the position is closed
        if (it->second.quantity < 1e-9) { // Use epsilon for floating point comparison
            Trade trade;
            trade.symbol = fill.symbol;
            // The original direction of the position being closed was BUY
            trade.direction = OrderDirection::BUY;
            trade.quantity = previous_quantity;
            trade.entry_price = it->second.average_cost;
            trade.exit_price = fill.fill_price;
            trade.pnl = pnl;
            // Timestamps would need to be tracked more accurately
            trade.entry_timestamp = 0; // Placeholder
            trade.exit_timestamp = fill.timestamp;
            
            trade_log.push_back(trade);
            holdings.erase(it);
        }
    }
    updateTimeIndex(); // Update equity after every fill
}

void Portfolio::onSignal(const SignalEvent& signal) {
    double order_quantity = 0.0;

    // --- Risk Management & Order Sizing ---
    double price = get_last_price(signal.symbol);
    if (price < 1e-9) {
        std::cerr << "Warning: No price available for " << signal.symbol << ". Cannot place order." << std::endl;
        return;
    }
    
    // Size based on stop loss if provided for a BUY signal
    if (signal.direction == OrderDirection::BUY && signal.stop_loss > 0) {
        double risk_per_share = price - signal.stop_loss;
        if (risk_per_share > 1e-9) {
            // Risk 1% of total equity on this trade
            double dollars_to_risk = get_total_equity() * 0.01;
            order_quantity = dollars_to_risk / risk_per_share;
        }
    }
    
    // Fallback to a simpler sizing model
    if (order_quantity < 1e-9) {
        // Use 5% of equity, scaled by signal strength
        order_quantity = (get_total_equity() * 0.05 * signal.strength) / price;
    }

    // --- Position & Order Logic ---
    auto it = holdings.find(signal.symbol);
    if (it != holdings.end() && it->second.quantity > 0) {
        if (signal.direction == OrderDirection::SELL) {
            // Signal to sell an existing position (exit)
            order_quantity = it->second.quantity;
        } else if (signal.direction == OrderDirection::BUY) {
            // Signal to buy more, but we already have a position.
            // A more complex strategy could scale into positions. For now, we ignore.
            return;
        }
    } else { // No position held
        if (signal.direction == OrderDirection::SELL) {
            // Signal to sell but we have no position (i.e., go short).
            // This template doesn't handle shorting, so we ignore.
            return;
        }
    }
    
    // Ensure order quantity is positive and we have enough cash
    if (order_quantity > 1e-9) {
         if (order_quantity * price > current_cash) {
            order_quantity = current_cash / price * 0.99; // Use 99% of available cash
        }
        auto order = std::make_shared<OrderEvent>(
            signal.symbol, 
            signal.timestamp, 
            OrderType::MARKET, 
            signal.direction, 
            order_quantity, 
            signal.stop_loss
        );
        event_queue->push(order);
    }
}

void Portfolio::onMarket(const MarketEvent& market) {
    auto it = holdings.find(market.symbol);
    if (it != holdings.end()) {
        it->second.market_value = it->second.quantity * market.price;
    }
}

void Portfolio::updateTimeIndex() {
    double total_market_value = 0.0;
    for (auto& pair : holdings) {
        Position& position = pair.second;
        // Use the latest price from the data handler for mark-to-market
        position.market_value = data_handler->getLatestPrice(position.symbol) * position.quantity;
        total_market_value += position.market_value;
    }

    total_equity = current_cash + total_market_value;
    
    // Update Drawdown
    peak_equity = std::max(peak_equity, total_equity);
    double current_drawdown = (peak_equity > 0) ? (peak_equity - total_equity) / peak_equity : 0.0;
    max_drawdown = std::max(max_drawdown, current_drawdown);
    
    // Add current equity to the time-series
    long long current_timestamp = data_handler->getLatestTimestamp();
    if (equity_curve.empty() || equity_curve.back().first != current_timestamp) {
        equity_curve.emplace_back(current_timestamp, total_equity);
    }
}

void Portfolio::generateReport() {
    std::cout << "\n<><><><><><><> PERFORMANCE REPORT <><><><><><><>\n" << std::endl;
    std::cout << std::fixed << std::setprecision(2);

    double gross_profit = 0.0;
    double gross_loss = 0.0;
    int winning_trades = 0;

    for (const auto& trade : trade_log) {
        if (trade.pnl > 0) {
            gross_profit += trade.pnl;
            winning_trades++;
        } else {
            gross_loss += trade.pnl;
        }
    }

    int total_trades = trade_log.size();
    int losing_trades = total_trades - winning_trades;
    double net_profit = gross_profit + gross_loss;
    double profit_factor = (gross_loss != 0) ? std::abs(gross_profit / gross_loss) : 99999.0;
    double win_rate = (total_trades > 0) ? static_cast<double>(winning_trades) / total_trades * 100.0 : 0.0;
    double avg_win = (winning_trades > 0) ? gross_profit / winning_trades : 0.0;
    double avg_loss = (losing_trades > 0) ? gross_loss / losing_trades : 0.0;

    std::cout << "--- Summary ---" << std::endl;
    std::cout << "Initial Capital:  " << initial_capital << std::endl;
    std::cout << "Final Equity:     " << total_equity << std::endl;
    std::cout << "Net Profit:       " << net_profit << " (" << (net_profit/initial_capital * 100.0) << "%)" << std::endl;
    std::cout << "Max Drawdown:     " << max_drawdown * 100.0 << "%" << std::endl;
    
    std::cout << "\n--- Trade Statistics ---" << std::endl;
    std::cout << "Total Trades:     " << total_trades << std::endl;
    std::cout << "Profit Factor:    " << profit_factor << std::endl;
    std::cout << "Win Rate:         " << win_rate << "%" << std::endl;
    std::cout << "Average Win:      " << avg_win << std::endl;
    std::cout << "Average Loss:     " << avg_loss << std::endl;
    std::cout << "Gross Profit:     " << gross_profit << std::endl;
    std::cout << "Gross Loss:       " << gross_loss << std::endl;

    std::cout << "\n<><><><><><><><><><><><><><><><><><><><><><><><>\n" << std::endl;
}

void Portfolio::writeResultsToCSV() {
    std::ofstream equity_file("equity_curve.csv");
    if (equity_file.is_open()) {
        equity_file << "timestamp,equity\n";
        for (const auto& point : equity_curve) {
            equity_file << point.first << "," << point.second << "\n";
        }
        equity_file.close();
        std::cout << "Equity curve saved to equity_curve.csv" << std::endl;
    } else {
        std::cerr << "Error: Unable to open equity_curve.csv for writing." << std::endl;
    }

    std::ofstream trades_file("trades_log.csv");
    if (trades_file.is_open()) {
        trades_file << "symbol,direction,quantity,entry_price,exit_price,pnl\n";
        for (const auto& trade : trade_log) {
            trades_file << trade.symbol << ","
                        << (trade.direction == OrderDirection::BUY ? "BUY" : "SELL") << ","
                        << trade.quantity << ","
                        << trade.entry_price << ","
                        << trade.exit_price << ","
                        << trade.pnl << "\n";
        }
        trades_file.close();
        std::cout << "Trade log saved to trades_log.csv" << std::endl;
    } else {
        std::cerr << "Error: Unable to open trades_log.csv for writing." << std::endl;
    }
}

const std::vector<std::pair<long long, double>>& Portfolio::getEquityCurve() const {
    return equity_curve;
}

std::string Portfolio::getPositionDirection(const std::string& symbol) const {
    auto it = holdings.find(symbol);
    if (it != holdings.end() && it->second.quantity > 0) {
        return "SELL"; // To close a long position
    }
    return "NONE";
}

double Portfolio::get_position(const std::string& symbol) const {
    auto it = holdings.find(symbol);
    if (it != holdings.end()) {
        return it->second.quantity;
    }
    return 0.0;
}

double Portfolio::get_last_price(const std::string& symbol) const {
    return data_handler->getLatestPrice(symbol);
}

double Portfolio::get_total_equity() const {
    return total_equity;
}
