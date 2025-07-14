#include "../../include/core/Performance.h"

Performance::Performance(const std::vector<double>& equity_curve, double initial_capital)
    : equity_curve(equity_curve), initial_capital(initial_capital) {
    if (equity_curve.empty() || initial_capital <= 0) {
        throw std::invalid_argument("Equity curve cannot be empty and initial capital must be positive.");
    }
}

double Performance::getTotalReturn() const {
    double final_value = equity_curve.back();
    return (final_value / initial_capital) - 1.0;
}

double Performance::getMaxDrawdown() const {
    double max_drawdown = 0.0;
    double peak_value = equity_curve[0];

    for (double value : equity_curve) {
        if (value > peak_value) {
            peak_value = value;
        }
        double drawdown = (peak_value - value) / peak_value;
        if (drawdown > max_drawdown) {
            max_drawdown = drawdown;
        }
    }
    return max_drawdown;
}

std::vector<double> Performance::calculateReturns() const {
    std::vector<double> returns;
    // We need at least two points to calculate one return value.
    if (equity_curve.size() < 2) {
        return returns;
    }

    for (size_t i = 1; i < equity_curve.size(); ++i) {
        double daily_return = (equity_curve[i] / equity_curve[i-1]) - 1.0;
        returns.push_back(daily_return);
    }
    return returns;
}

double Performance::getSharpeRatio(double risk_free_rate) const {
    std::vector<double> returns = calculateReturns();
    if (returns.size() < 2) {
        return 0.0; // Not enough data to calculate Sharpe Ratio
    }

    // Calculate mean of returns
    double sum_of_returns = std::accumulate(returns.begin(), returns.end(), 0.0);
    double mean_return = sum_of_returns / returns.size();

    // Calculate standard deviation of returns
    double sq_sum = 0.0;
    for (double r : returns) {
        sq_sum += (r - mean_return) * (r - mean_return);
    }
    double std_dev = std::sqrt(sq_sum / returns.size());

    if (std_dev < 1e-8) {
        return 0.0; // Avoid division by zero if returns are flat
    }

    // Annualize the Sharpe Ratio. Assumes 252 trading days in a year.
    // Adjust this factor if your data is weekly, hourly, etc.
    const double annualization_factor = 252.0;
    double sharpe_ratio = (mean_return - risk_free_rate) / std_dev;
    
    return sharpe_ratio * std::sqrt(annualization_factor);
}