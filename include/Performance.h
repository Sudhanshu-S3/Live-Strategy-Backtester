#ifndef PERFORMANCE_H
#define PERFORMANCE_H

#include <vector>
#include <cstdint>
using namespace std;
class Performance {
public:
    
    Performance(const vector<pair<uint64_t, double>>& equity_curve, double initial_capital);

    double getTotalReturn() const;

    double getMaxDrawdown() const;

    double getSharpeRatio() const;

private:
    double total_return;
    double max_drawdown;
    double sharpe_ratio;

    void calculateMetrics(const vector<pair<uint64_t, double>>& equity_curve, double initial_capital);
};

#endif