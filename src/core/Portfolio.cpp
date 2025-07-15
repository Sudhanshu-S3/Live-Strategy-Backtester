#include "../../include/core/Portfolio.h"
#include "../../include/event/Event.h"
#include <iostream>
#include <numeric>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include "../../include/core/Performance.h"

Portfolio::Portfolio(
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
    std::shared_ptr<DataHandler> data_handler,
    double initial_capital
) : event_queue_(event_queue),
    data_handler_(data_handler),
    initial_capital_(initial_capital),
    total_equity_(initial_capital),
    cash_(initial_capital),
    max_drawdown_(0.0) {}

void Portfolio::onFill(const FillEvent& fill_event) {
    double cost = fill_event.fill_price * fill_event.quantity;
    
    if (fill_event.direction == OrderDirection::BUY) {
        cash_ -= cost;
        holdings_[fill_event.symbol].quantity += fill_event.quantity;
        holdings_[fill_event.symbol].avg_price = 
            (holdings_[fill_event.symbol].avg_price * (holdings_[fill_event.symbol].quantity - fill_event.quantity) + cost) / holdings_[fill_event.symbol].quantity;
    } else { // SELL
        cash_ += cost;
        holdings_[fill_event.symbol].quantity -= fill_event.quantity;
    }

    // STAGE 4: Log the trade for performance attribution
    logTrade(fill_event);

    updatePortfolioValue();
}

void Portfolio::onMarket(const MarketEvent& market_event) {
    updatePortfolioValue();
}

void Portfolio::updatePortfolioValue() {
    double holdings_value = 0.0;
    for (auto const& [symbol, position] : holdings_) {
        double market_price = data_handler_->getLatestBarValue(symbol, "price");
        if(market_price > 0) {
            holdings_value += position.quantity * market_price;
        } else { // If price is not available, use average cost
            holdings_value += position.quantity * position.avg_price;
        }
    }

    total_equity_ = cash_ + holdings_value;
    
    // Update equity curve
    // In a real system, you'd get a proper timestamp.
    std::string current_timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    equity_curve.emplace_back(current_timestamp, total_equity_);
    
    // Update max drawdown
    if (total_equity_ > peak_equity_) {
        peak_equity_ = total_equity_;
    }
    double drawdown = (peak_equity_ - total_equity_) / peak_equity_;
    if (drawdown > max_drawdown_) {
        max_drawdown_ = drawdown;
    }
}

// STAGE 4: New method to log individual trades
void Portfolio::logTrade(const FillEvent& fill) {
    // This is a simplified logger. A real one would match buys and sells to create closed trades.
    // For now, we log each fill as a "trade".
    TradeRecord trade;
    trade.symbol = fill.symbol;
    trade.direction = fill.direction;
    trade.quantity = fill.quantity;
    trade.entry_price = fill.fill_price;
    trade.exit_price = fill.fill_price; // For a single fill, entry=exit
    
    // PnL calculation is more complex and requires matching trades.
    // For now, PnL is 0 for a single fill event. Real PnL is tracked via equity.
    trade.pnl = 0; 
    trade.entry_timestamp = fill.timestamp;
    trade.exit_timestamp = fill.timestamp;
    
    trade_log.push_back(trade);
}

void Portfolio::generateReport() {
    std::cout << "\n--- Portfolio Performance Summary ---\n";
    std::cout << "Initial Capital: $" << initial_capital_ << std::endl;
    std::cout << "Final Equity: $" << total_equity_ << std::endl;

    std::vector<double> equity_values;
    for(const auto& p : equity_curve) {
        equity_values.push_back(p.second);
    }
    
    if (equity_values.size() < 2) {
        std::cout << "Not enough data for detailed performance metrics." << std::endl;
        return;
    }

    Performance perf(equity_values, initial_capital_, trade_log); // STAGE 4: Pass trade log
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total Return: " << perf.getTotalReturn() * 100.0 << "%" << std::endl;
    std::cout << "Max Drawdown: " << perf.getMaxDrawdown() * 100.0 << "%" << std::endl;
    std::cout << "Sharpe Ratio (Annualized): " << perf.getSharpeRatio() << std::endl;

    // STAGE 4: Trade-level report
    generateTradeLevelReport(perf);

    // STAGE 5: Monte Carlo Simulation
    perf.runMonteCarloSimulation(1000);

    std::cout << "-------------------------------------\n";
}

void Portfolio::generateTradeLevelReport(const Performance& perf) const {
    std::cout << "\n--- Trade-Level Analysis ---\n";
    if (trade_log.empty()) {
        std::cout << "No trades were made." << std::endl;
        return;
    }
    
    // These metrics are now calculated in Performance class
    std::cout << "Total Trades: " << perf.getTotalTrades() << std::endl;
    std::cout << "Winning Trades: " << perf.getWinningTrades() << std::endl;
    std::cout << "Losing Trades: " << perf.getLosingTrades() << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Win Rate: " << perf.getWinRate() << "%" << std::endl;
    std::cout << "Profit Factor: " << perf.getProfitFactor() << std::endl;
}

// STAGE 3: Update portfolio with strategy status
void Portfolio::updateStrategyStatus(const std::string& strategy_name, const std::string& status) {
    std::cout << "Portfolio Notified: Strategy '" << strategy_name << "' status changed to " << status << std::endl;
    // In a real system, this might trigger specific portfolio-level actions,
    // like liquidating positions opened by that strategy.
    strategy_status_[strategy_name] = status;
}

void Portfolio::updateTimeIndex() { /* For future use */ }
double Portfolio::getInitialCapital() const { return initial_capital_; }
const std::vector<std::pair<std::string, double>>& Portfolio::getEquityCurve() const { return equity_curve; }