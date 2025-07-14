#include "Performance.h"
#include <numeric>   
#include <cmath>     
#include <algorithm> 
using namespace std;

Performance::Performance(const vector<pair<uint64_t, double>>& equity_curve, double initial_capital)
    : total_return(0.0), max_drawdown(0.0), sharpe_ratio(0.0) {
    if (!equity_curve.empty()) {
        calculateMetrics(equity_curve, initial_capital);
    }
}

void Performance::calculateMetrics(const vector<pair<uint64_t, double>>& equity_curve, double initial_capital) {
    // Calculate Total Return 
    double final_value = equity_curve.back().second;
    this->total_return = (final_value - initial_capital) / initial_capital;

    // Calculate Maximum Drawdown 
    double peak = -1.0;
    for (const auto& point : equity_curve) {
        double current_value = point.second;
        if (current_value > peak) {
            peak = current_value;
        }
        double drawdown = (peak - current_value) / peak;
        if (drawdown > this->max_drawdown) {
            this->max_drawdown = drawdown;
        }
    }

    // Calculate Annualized Sharpe Ratio 
    
    vector<double> returns;
    for (size_t i = 1; i < equity_curve.size(); ++i) {
        double prev_value = equity_curve[i - 1].second;
        double current_value = equity_curve[i].second;
        if (prev_value != 0) {
            returns.push_back((current_value / prev_value) - 1.0);
        }
    }

    if (returns.size() < 2) {
        this->sharpe_ratio = 0.0; 
        return;
    }

    // Calculate the mean of the returns
    double sum_of_returns = accumulate(returns.begin(), returns.end(), 0.0);
    double mean_return = sum_of_returns / returns.size();

    // Calculate the standard deviation of the returns
    double sum_of_squared_diffs = 0.0;
    for (const double r : returns) {
        sum_of_squared_diffs += pow(r - mean_return, 2);
    }
    double std_dev = sqrt(sum_of_squared_diffs / returns.size());

    // Calculate and annualize the Sharpe Ratio (assuming 0 risk-free rate)
    // For hourly data, there are 24 * 365 = 8760 periods in a year.
    if (std_dev != 0) {
        double hourly_sharpe = mean_return / std_dev;
        this->sharpe_ratio = hourly_sharpe * sqrt(8760);
    } else {
        this->sharpe_ratio = 0.0; 
    }
}


double Performance::getTotalReturn() const {
    return this->total_return;
}

double Performance::getMaxDrawdown() const {
    return this->max_drawdown;
}

double Performance::getSharpeRatio() const {
    return this->sharpe_ratio;
}