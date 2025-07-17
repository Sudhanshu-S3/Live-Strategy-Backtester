#include "risk/SharpeRatio.h"
#include <numeric>
#include <stdexcept>

SharpeRatio::SharpeRatio(double risk_free_rate)
    : annual_risk_free_rate_(risk_free_rate) {}

double SharpeRatio::calculate(const std::vector<double>& returns, int periods_per_year) const {
    if (returns.size() < 2) {
        return 0.0; // Not enough data for standard deviation
    }

    // 1. Calculate mean of returns
    double sum = std::accumulate(returns.begin(), returns.end(), 0.0);
    double mean_return = sum / returns.size();

    // 2. Calculate standard deviation of returns
    double sq_sum = std::inner_product(returns.begin(), returns.end(), returns.begin(), 0.0);
    double std_dev = std::sqrt(sq_sum / returns.size() - mean_return * mean_return);

    if (std_dev < 1e-9) {
        return 0.0; // Avoid division by zero if there is no variance in returns
    }

    // 3. Calculate excess return and annualize
    double periodic_risk_free_rate = annual_risk_free_rate_ / periods_per_year;
    double excess_return = mean_return - periodic_risk_free_rate;

    // 4. Annualize the Sharpe Ratio
    return (excess_return / std_dev) * std::sqrt(periods_per_year);
}