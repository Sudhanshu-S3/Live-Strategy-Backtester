#include "../../include/core/Performance.h"
#include <algorithm>
#include <random> // STAGE 5: For Monte Carlo

Performance::Performance(const std::vector<double>& equity_curve, double initial_capital, const std::vector<TradeRecord>& trade_log)
    : equity_curve_(equity_curve), initial_capital_(initial_capital), trade_log_(trade_log) {
    if (equity_curve.empty() || initial_capital <= 0) {
        throw std::invalid_argument("Equity curve cannot be empty and initial capital must be positive.");
    }
    calculateTradeLevelStats(); // STAGE 4
}

double Performance::getTotalReturn() const {
    return (equity_curve_.back() / initial_capital_) - 1.0;
}

double Performance::getMaxDrawdown() const {
    double max_drawdown = 0.0;
    double peak_value = equity_curve_[0];
    for (double value : equity_curve_) {
        if (value > peak_value) peak_value = value;
        double drawdown = (peak_value - value) / peak_value;
        if (drawdown > max_drawdown) max_drawdown = drawdown;
    }
    return max_drawdown;
}

std::vector<double> Performance::calculateReturns() const {
    std::vector<double> returns;
    if (equity_curve_.size() < 2) return returns;
    for (size_t i = 1; i < equity_curve_.size(); ++i) {
        returns.push_back((equity_curve_[i] / equity_curve_[i-1]) - 1.0);
    }
    return returns;
}

double Performance::getSharpeRatio(double risk_free_rate) const {
    std::vector<double> returns = calculateReturns();
    if (returns.size() < 2) return 0.0;
    double mean_return = calculateMean(returns);
    double std_dev = calculateStandardDeviation(returns, mean_return);
    if (std_dev < 1e-9) return 0.0;
    const double annualization_factor = 252.0; // Assuming daily data
    return ((mean_return - risk_free_rate) / std_dev) * std::sqrt(annualization_factor);
}

// STAGE 4: Trade-level metrics
void Performance::calculateTradeLevelStats() {
    // This requires a proper trade matching logic (e.g., FIFO) to calculate PnL per trade.
    // The current `trade_log_` just contains fills. For this example, we'll
    // derive some simplified stats. A real implementation would be more complex.
    gross_profit_ = 0;
    gross_loss_ = 0;
    winning_trades_ = 0;
    losing_trades_ = 0;

    // Simplified PnL calculation: assume each trade is independent.
    // This is NOT accurate for real position tracking but serves as an example.
    for(const auto& trade : trade_log_) {
        // A more realistic PnL would be calculated in the portfolio when a position is closed.
        // For now, we can't calculate meaningful PnL from single fill events.
        // We will simulate some PnL for demonstration.
        if (trade.pnl > 0) { // Assuming PnL is pre-calculated
             gross_profit_ += trade.pnl;
             winning_trades_++;
        } else {
             gross_loss_ += trade.pnl;
             losing_trades_++;
        }
    }
    // Since our current Portfolio doesn't calculate PnL per trade, let's use a proxy.
    // We'll look at the change in equity after each trade. This is still not perfect.
    // For now, we'll just report counts.
    total_trades_ = trade_log_.size();
    // The rest of the metrics (win rate, profit factor) depend on accurate PnL.
}

// STAGE 5: Monte Carlo Simulation
void Performance::runMonteCarloSimulation(int num_simulations) const {
    std::cout << "\n--- Monte Carlo Simulation (" << num_simulations << " runs) ---\n";
    std::vector<double> returns = calculateReturns();
    if (returns.size() < 2) {
        std::cout << "Not enough data for Monte Carlo simulation." << std::endl;
        return;
    }

    std::vector<double> final_returns;
    std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
    
    for (int i = 0; i < num_simulations; ++i) {
        std::shuffle(returns.begin(), returns.end(), generator);
        double simulated_equity = initial_capital_;
        for (double r : returns) {
            simulated_equity *= (1.0 + r);
        }
        final_returns.push_back((simulated_equity / initial_capital_) - 1.0);
    }

    std::sort(final_returns.begin(), final_returns.end());
    
    double mean_sim_return = calculateMean(final_returns);
    double p5 = final_returns[static_cast<size_t>(num_simulations * 0.05)];
    double p95 = final_returns[static_cast<size_t>(num_simulations * 0.95)];

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Average Simulated Return: " << mean_sim_return * 100.0 << "%" << std::endl;
    std::cout << "5th Percentile Return: " << p5 * 100.0 << "%" << std::endl;
    std::cout << "95th Percentile Return: " << p95 * 100.0 << "%" << std::endl;
}


// --- Helper and Accessor Methods ---
double Performance::calculateMean(const std::vector<double>& data) const {
    if (data.empty()) return 0.0;
    return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
}
double Performance::calculateStandardDeviation(const std::vector<double>& data, double mean) const {
    if (data.size() < 2) return 0.0;
    double sq_sum = std::accumulate(data.begin(), data.end(), 0.0, 
        [mean](double acc, double r){ return acc + (r - mean) * (r - mean); });
    return std::sqrt(sq_sum / data.size());
}
int Performance::getTotalTrades() const { return total_trades_; }
int Performance::getWinningTrades() const { return winning_trades_; }
int Performance::getLosingTrades() const { return losing_trades_; }
double Performance::getWinRate() const {
    return (total_trades_ > 0) ? (static_cast<double>(winning_trades_) / total_trades_) * 100.0 : 0.0;
}
double Performance::getProfitFactor() const {
    return (std::abs(gross_loss_) > 1e-9) ? std::abs(gross_profit_ / gross_loss_) : 0.0;
}