#ifndef PERFORMANCE_H
#define PERFORMANCE_H

#include <vector>
#include <numeric> // For std::accumulate
#include <cmath>   // For std::sqrt
#include <stdexcept>

class Performance {
public:
    // Constructor takes the equity curve and initial capital.
    // The equity curve is taken by const reference for efficiency.
    Performance(const std::vector<double>& equity_curve, double initial_capital);

    // Public methods to get the calculated performance metrics.
    double getTotalReturn() const;
    double getMaxDrawdown() const;
    double getSharpeRatio(double risk_free_rate = 0.0) const;

private:
    std::vector<double> equity_curve;
    double initial_capital;

    // A helper to calculate daily/periodic returns from the equity curve.
    std::vector<double> calculateReturns() const;
};

#endif // PERFORMANCE_H