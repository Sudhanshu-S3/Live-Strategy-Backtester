# Performance Measurement

## Overview

This document outlines the key performance metrics used to evaluate trading strategies in the Live Strategy Backtester framework.

## Core Metrics

### Return Metrics

- **Total Return**: Overall percentage gain/loss of the strategy
- **Annualized Return**: Return normalized to a yearly rate
- **Daily/Monthly Returns**: Periodic return breakdown

### Risk Metrics

- **Maximum Drawdown**: Largest peak-to-trough decline in portfolio value
- **Volatility**: Standard deviation of returns
- **Sharpe Ratio**: Average return earned in excess of the risk-free rate per unit of volatility
- **Sortino Ratio**: Similar to Sharpe ratio but only considering downside volatility

### Trading Metrics

- **Win Rate**: Percentage of profitable trades
- **Profit Factor**: Gross profit divided by gross loss
- **Average Trade**: Mean P&L per trade
- **Average Win/Loss**: Mean P&L for winning and losing trades
- **Trade Duration**: Average holding period for positions

## Advanced Metrics

### Risk-Adjusted Performance

- **Calmar Ratio**: Annual return divided by maximum drawdown
- **Omega Ratio**: Probability-weighted ratio of gains versus losses
- **Alpha**: Excess return compared to a benchmark

### Market Condition Analysis

- **Beta**: Correlation of strategy returns with market returns
- **Correlation Matrix**: Relationship between strategy and various market indices

## Visualization Tools

The framework provides several visualization tools for performance analysis:

- Equity curves
- Drawdown charts
- Monthly/yearly return heatmaps
- Return distribution histograms

## Custom Metric Implementation

To implement custom performance metrics, use the `PerformanceAnalytics` class:

```cpp
#include "analytics/PerformanceAnalytics.h"

// Example: Creating a custom risk-adjusted metric
class CustomMetric : public IPerformanceMetric {
public:
    double calculate(const std::vector<Trade>& trades) override {
        // Implement custom calculation logic
        return result;
    }
};
```
