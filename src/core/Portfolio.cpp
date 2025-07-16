#include "../../include/core/Portfolio.h"
#include "../../include/event/Event.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <numeric>
#include <cmath>

Portfolio::Portfolio(
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
    double initial_capital,
    std::shared_ptr<DataHandler> data_handler
) : event_queue_(event_queue),
    initial_capital_(initial_capital),
    data_handler_(data_handler),
    total_equity_(initial_capital),
    current_cash_(initial_capital),
    peak_equity_(initial_capital),
    max_drawdown_(0.0) {}

void Portfolio::onSignal(const SignalEvent& signal) {
    // This is handled by the RiskManager now, Portfolio does not need to generate orders.
}

void Portfolio::onMarketRegimeChanged(const MarketRegimeChangedEvent& event) {
    current_market_state_ = event.new_state;
}

void Portfolio::onFill(const FillEvent& fill_event) {
    double cost = fill_event.fill_price * fill_event.quantity;

    if (fill_event.direction == OrderDirection::BUY) {
        current_cash_ -= cost;
    } else { // SELL
        current_cash_ += cost;
    }

    Position& position = holdings_[fill_event.symbol];
    position.symbol = fill_event.symbol;

    double old_quantity = position.quantity;
    if (fill_event.direction == OrderDirection::BUY) {
        position.quantity += fill_event.quantity;
        position.direction = OrderDirection::BUY;
        double old_total_cost = position.average_cost * old_quantity;
        position.average_cost = (old_total_cost + cost) / position.quantity;
    } else { // SELL
        position.quantity -= fill_event.quantity;
    }

    if (std::abs(position.quantity) < 1e-9) { // Position is closed
        // --- PNL Calculation for Closed Trade ---
        double pnl = 0.0;
        if (fill_event.direction == OrderDirection::SELL) { // Closed a long position
            pnl = (fill_event.fill_price - position.average_cost) * fill_event.quantity;
        } else { // Closed a short position
            pnl = (position.average_cost - fill_event.fill_price) * fill_event.quantity;
        }
        
        // Find the original entry trade and update its PnL
        // This is a simplification. A real system would handle partial fills and multiple entries.
        for (auto it = trade_log_.rbegin(); it != trade_log_.rend(); ++it) {
            if (it->symbol == fill_event.symbol && it->pnl == 0.0) {
                it->pnl = pnl;
                it->exit_price = fill_event.fill_price;
                it->exit_timestamp = fill_event.timestamp;
                break;
            }
        }
        for (auto& [name, trades] : strategy_trade_log_) {
             for (auto it = trades.rbegin(); it != trades.rend(); ++it) {
                if (it->symbol == fill_event.symbol && it->pnl == 0.0) {
                    it->pnl = pnl;
                    it->exit_price = fill_event.fill_price;
                    it->exit_timestamp = fill_event.timestamp;
                    break;
                }
            }
        }

        position.direction = OrderDirection::NONE;
        position.average_cost = 0.0;
    }

    Trade trade;
    trade.symbol = fill_event.symbol;
    trade.direction = fill_event.direction;
    trade.quantity = fill_event.quantity;
    trade.entry_price = fill_event.fill_price;
    trade.entry_timestamp = fill_event.timestamp;
    trade.market_state_at_entry = current_market_state_;
    trade_log_.push_back(trade);
    
    if (!fill_event.strategy_name.empty()) {
        strategy_trade_log_[fill_event.strategy_name].push_back(trade);
    }

    updateTimeIndex();
}

void Portfolio::onMarket(const MarketEvent& market_event) {
    updateTimeIndex();
}

void Portfolio::updateTimeIndex() {
    double holdings_value = 0.0;
    for (auto& [symbol, position] : holdings_) {
        double market_price = data_handler_->getLatestBarValue(symbol, "price");
        if (market_price > 0) {
            position.market_value = position.quantity * market_price;
            holdings_value += position.market_value;
        } else {
            holdings_value += position.quantity * position.average_cost;
        }
    }

    total_equity_ = current_cash_ + holdings_value;

    equity_curve_.emplace_back(std::chrono::system_clock::now().time_since_epoch().count(), total_equity_, current_market_state_);

    if (total_equity_ > peak_equity_) {
        peak_equity_ = total_equity_;
    }
    double drawdown = (peak_equity_ - total_equity_) / peak_equity_;
    if (drawdown > max_drawdown_) {
        max_drawdown_ = drawdown;
    }
}

void Portfolio::generateReport() {
    std::cout << "\n--- Portfolio Performance Summary ---\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Initial Capital: $" << initial_capital_ << std::endl;
    std::cout << "Final Equity:    $" << total_equity_ << std::endl;

    std::vector<double> equity_values;
    for (const auto& p : equity_curve_) {
        equity_values.push_back(std::get<1>(p));
    }

    if (equity_values.size() < 2) {
        std::cout << "Not enough data for detailed performance metrics." << std::endl;
        return;
    }

    Performance perf(equity_values, initial_capital_, trade_log_);
    std::cout << "Total Return: " << perf.getTotalReturn() * 100.0 << "%" << std::endl;
    std::cout << "Max Drawdown: " << perf.getMaxDrawdown() * 100.0 << "%" << std::endl;
    std::cout << "Sharpe Ratio: " << perf.getSharpeRatio() << std::endl;

    generateTradeLevelReport();
    std::cout << "-------------------------------------\n";
}

void Portfolio::writeResultsToCSV(const std::string& filename) {
    std::ofstream file(filename);
    file << "timestamp,equity,vol_regime,trend_regime\n";
    for (const auto& point : equity_curve_) {
        file << std::get<0>(point) << "," 
             << std::get<1>(point) << ","
             << static_cast<int>(std::get<2>(point).volatility) << ","
             << static_cast<int>(std::get<2>(point).trend) << "\n";
    }
}

void Portfolio::writeTradeLogToCSV(const std::string& filename) {
    std::ofstream file(filename);
    file << "strategy,symbol,direction,quantity,entry_price,entry_timestamp,volatility,trend\n";
    for (const auto& [strategy_name, trades] : strategy_trade_log_) {
        for (const auto& trade : trades) {
            file << strategy_name << ","
                 << trade.symbol << ","
                 << (trade.direction == OrderDirection::BUY ? "BUY" : "SELL") << ","
                 << trade.quantity << ","
                 << trade.entry_price << ","
                 << trade.entry_timestamp << ","
                 << static_cast<int>(trade.market_state_at_entry.volatility) << ","
                 << static_cast<int>(trade.market_state_at_entry.trend) << "\n";
        }
    }
}

double Portfolio::get_total_equity() const { return total_equity_; }

const std::vector<std::tuple<long long, double, MarketState>>& Portfolio::getEquityCurve() const {
    return equity_curve_;
}

std::string Portfolio::getPositionDirection(const std::string& symbol) const {
    auto it = holdings_.find(symbol);
    if (it != holdings_.end()) {
        if (it->second.direction == OrderDirection::BUY) return "LONG";
        if (it->second.direction == OrderDirection::SELL) return "SHORT";
    }
    return "NONE";
}

double Portfolio::get_position(const std::string& symbol) const {
    auto it = holdings_.find(symbol);
    return (it != holdings_.end()) ? it->second.quantity : 0.0;
}

double Portfolio::get_last_price(const std::string& symbol) const {
    return data_handler_->getLatestBarValue(symbol, "price");
}

double Portfolio::getRealTimePnL() const {
    return total_equity_ - initial_capital_;
}

std::map<std::string, Position> Portfolio::getCurrentPositions() const {
    return holdings_;
}

Performance Portfolio::getRealTimePerformance() const {
    std::vector<double> equity_values;
    for (const auto& p : equity_curve_) {
        equity_values.push_back(std::get<1>(p));
    }
    return Performance(equity_values, initial_capital_, trade_log_);
}

void Portfolio::generateTradeLevelReport() const {
    std::cout << "\n--- Trade Log ---\n";
    if (trade_log_.empty()) {
        std::cout << "No trades were made." << std::endl;
        return;
    }
    std::cout << "Total Trades: " << trade_log_.size() << std::endl;
}