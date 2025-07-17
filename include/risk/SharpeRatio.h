#ifndef SHARPE_RATIO_H
#define SHARPE_RATIO_H

#include <vector>
#include <cmath>

/**
 * @class SharpeRatio
 * @brief Calculates the Sharpe ratio for a series of returns.
 */
class SharpeRatio {
public:
    /**
     * @brief Construct a new Sharpe Ratio calculator.
     * @param risk_free_rate The annual risk-free rate (e.g., 0.02 for 2%).
     */
    explicit SharpeRatio(double risk_free_rate);

    /**
     * @brief Calculates the annualized Sharpe ratio from a vector of periodic returns.
     * @param returns A vector of periodic returns (e.g., daily).
     * @param periods_per_year The number of periods in a year (e.g., 252 for daily trading days).
     * @return The annualized Sharpe ratio. Returns 0.0 if calculation is not possible.
     */
    double calculate(const std::vector<double>& returns, int periods_per_year = 252) const;

private:
    double annual_risk_free_rate_;
};

#endif // SHARPE_RATIO_H