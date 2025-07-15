#ifndef PERFORMANCE_H
#define PERFORMANCE_H

#include <vector>
#include <numeric> // For std::accumulate
#include <cmath>   // For std::sqrt
#include <stdexcept>
#include <map>     // Required for correlation

class Performance {
public:
    // Constructor takes the equity curve and initial capital.
    // The equity curve is taken by const reference for efficiency.
    Performance(const std::vector<double>& equity_curve, double initial_capital);

    // Public methods to get the calculated performance metrics.
    double getTotalReturn() const;
    double getMaxDrawdown() const;
    double getSharpeRatio(double risk_free_rate = 0.0) const;

    // New methods for risk analytics and correlation
    double calculateVaR(double confidence_level = 0.95) const;
    double calculateBeta(const std::vector<double>& benchmark_returns) const;
    double calculateCorrelation(const std::vector<double>& other_returns) const;

private:
    std::vector<double> equity_curve;
    double initial_capital;

    // A helper to calculate daily/periodic returns from the equity curve.
    std::vector<double> calculateReturns() const;

    // Helper for mean
    double calculateMean(const std::vector<double>& data) const;
    // Helper for standard deviation
    double calculateStandardDeviation(const std::vector<double>& data, double mean) const;
};

#endif // PERFORMANCE_H