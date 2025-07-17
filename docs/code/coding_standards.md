# Coding Standards and Documentation Guide

## Overview

This document outlines the coding standards and documentation requirements for the Live Strategy Backtester project. Consistent code style and thorough documentation are essential for maintainability, extensibility, and team collaboration.

## C++ Coding Standards

### Code Formatting

#### General Style

```cpp
// Right way
namespace backtester {

class StrategyExecutor {
public:
    StrategyExecutor(const StrategyConfig& config);

    void execute(const MarketData& data);

private:
    double calculateSignal(const MarketData& data) const;

    StrategyConfig m_config;
    PositionManager m_positions;
};

} // namespace backtester
```

#### Naming Conventions

- **Namespaces**: `lowercase`
- **Classes and Types**: `CamelCase`
- **Methods and Functions**: `camelCase`
- **Variables**: `camelCase`
- **Class Member Variables**: `m_camelCase`
- **Constants**: `UPPER_SNAKE_CASE`
- **Template Parameters**: `CamelCase` or `T` for simple templates

#### Indentation and Spacing

- Use 4 spaces for indentation (no tabs)
- Place braces on their own lines for namespaces, classes, and functions
- Use inline braces for control structures (if, while, for)
- Add space after keywords and before opening parenthesis
- No space between function name and opening parenthesis

```cpp
// Right way
if (condition) {
    doSomething();
} else {
    doSomethingElse();
}

for (int i = 0; i < limit; i++) {
    process(i);
}

void calculateMetrics() {
    // Implementation
}
```

### Documentation Standards

#### File Headers

Every file should start with a standard header comment:

```cpp
/**
 * @file StrategyExecutor.h
 * @brief Defines the StrategyExecutor class responsible for executing trading strategies
 *
 * @author Developer Name
 * @date 2025-07-10
 */
```

#### Class Documentation

Document each class with a description, purpose, and usage:

```cpp
/**
 * @class StrategyExecutor
 * @brief Executes trading strategies based on market data
 *
 * The StrategyExecutor is responsible for applying a trading strategy to
 * market data and generating trading signals. It handles the execution
 * logic, signal generation, and integration with the position manager.
 *
 * @note This class is not thread-safe and should be used from a single thread.
 *
 * @see StrategyConfig
 * @see PositionManager
 */
class StrategyExecutor {
    // Class implementation
};
```

#### Method Documentation

Document each method with a description, parameters, return value, and exceptions:

```cpp
/**
 * @brief Calculate the trading signal based on current market data
 *
 * This method implements the core strategy logic, analyzing the provided
 * market data and calculating a normalized signal value in the range [-1, 1],
 * where:
 *   - -1 represents maximum bearish signal
 *   -  0 represents neutral signal
 *   -  1 represents maximum bullish signal
 *
 * @param data The current market data containing price, volume, and indicators
 * @return double The calculated signal value in the range [-1, 1]
 *
 * @throw std::invalid_argument If the market data object is missing required fields
 *
 * @pre The market data must contain at least one valid price point
 * @post The return value will be in the range [-1, 1]
 */
double calculateSignal(const MarketData& data) const;
```

#### Variable Documentation

Document important variables and complex data structures:

```cpp
//! Configuration parameters for the strategy
StrategyConfig m_config;

//! Manages current positions and pending orders
PositionManager m_positions;

//! Tracks performance metrics for the current execution
PerformanceTracker m_performance;

//! Map of asset IDs to their corresponding volatility metrics
std::unordered_map<AssetId, VolatilityMetrics> m_volatilityMap;
```

#### Inline Comments

Use inline comments to explain complex logic or non-obvious code:

```cpp
// Calculate Bollinger Bands with 2 standard deviations
double mean = calculateSMA(prices, 20);
double stdDev = calculateStdDev(prices, 20, mean);
double upperBand = mean + (2.0 * stdDev);
double lowerBand = mean - (2.0 * stdDev);

// Check for a breakout pattern
bool breakoutUp = price > upperBand && previousPrice <= previousUpperBand;
bool breakoutDown = price < lowerBand && previousPrice >= previousLowerBand;

// Position sizing based on volatility - use smaller position sizes
// during high volatility periods to manage risk
double positionSize = baseSize * (maxVolatility / currentVolatility);

// Apply a time decay factor to the signal to reduce position size
// as we approach market close
if (timeToMarketClose < decayThreshold) {
    signal *= (timeToMarketClose / decayThreshold);
}
```

### Best Practices for Documentation

#### 1. Document Why, Not What

Focus on explaining why the code does something rather than what it does, as the what should be evident from reading the code itself:

```cpp
// Bad comment - explains what is obvious from the code
price = price + adjustment; // Add adjustment to price

// Good comment - explains the why
price = price + adjustment; // Adjust price to account for dividend events
```

#### 2. Document Assumptions

Clearly document any assumptions that the code makes:

```cpp
// Assuming input prices are in ascending chronological order
double calculateReturns(const std::vector<double>& prices) {
    // Implementation
}
```

#### 3. Document Limitations

Note any limitations or edge cases:

```cpp
/**
 * @brief Calculate the Sharpe ratio of the returns
 *
 * @note This implementation assumes daily returns and annualizes with
 *       252 trading days. For other frequencies, use the overloaded version
 *       with a custom annualization factor.
 *
 * @warning Requires at least 30 data points for statistical significance
 */
double calculateSharpeRatio(const std::vector<double>& returns);
```

#### 4. Document Performance Considerations

Include notes about performance characteristics for critical sections:

```cpp
/**
 * @brief Optimize the strategy parameters using grid search
 *
 * @note Time complexity: O(n^p) where n is the grid resolution and
 *       p is the number of parameters. Memory usage: O(n^p).
 *       For high-dimensional parameter spaces, consider using
 *       RandomSearch or BayesianOptimizer instead.
 */
std::vector<double> optimizeParameters();
```

### Example: Complete Class Documentation

Here's a complete example of a well-documented class:

```cpp
/**
 * @file RiskManager.h
 * @brief Defines the RiskManager class for managing trading risk
 *
 * @author Trading Systems Team
 * @date 2025-07-15
 */

#ifndef RISK_MANAGER_H
#define RISK_MANAGER_H

#include <vector>
#include <unordered_map>
#include "Position.h"
#include "RiskConfig.h"

namespace backtester {
namespace risk {

/**
 * @class RiskManager
 * @brief Manages risk for a trading portfolio
 *
 * The RiskManager is responsible for evaluating risk metrics and
 * enforcing risk limits on a trading portfolio. It can calculate
 * various risk measures such as Value at Risk (VaR), portfolio volatility,
 * drawdown, and concentration risk.
 *
 * @see RiskConfig
 * @see Position
 */
class RiskManager {
public:
    /**
     * @brief Construct a new Risk Manager
     *
     * @param config The risk configuration parameters
     */
    explicit RiskManager(const RiskConfig& config);

    /**
     * @brief Check if a new position would violate risk limits
     *
     * Evaluates whether adding a new position or modifying an existing
     * position would violate any of the configured risk limits.
     *
     * @param position The position to be evaluated
     * @param currentPortfolio The current portfolio state
     * @return bool True if the position is acceptable, false if it violates limits
     */
    bool checkPositionRisk(const Position& position,
                           const std::vector<Position>& currentPortfolio);

    /**
     * @brief Calculate the maximum position size allowed by risk constraints
     *
     * Determines the maximum position size that would be allowed for a given
     * asset based on current portfolio state and risk configuration.
     *
     * @param assetId The identifier for the asset
     * @param price The current price of the asset
     * @param volatility The current volatility of the asset
     * @param currentPortfolio The current portfolio state
     * @return double The maximum allowed position size
     */
    double calculateMaxPositionSize(const std::string& assetId,
                                   double price,
                                   double volatility,
                                   const std::vector<Position>& currentPortfolio);

    /**
     * @brief Calculate the current Value at Risk (VaR) for the portfolio
     *
     * @param positions The current portfolio positions
     * @param confidenceLevel The confidence level for VaR calculation (default: 0.95)
     * @param timeHorizon The time horizon in days (default: 1)
     * @return double The calculated VaR value
     */
    double calculateValueAtRisk(const std::vector<Position>& positions,
                               double confidenceLevel = 0.95,
                               int timeHorizon = 1);

private:
    /**
     * @brief Calculate the correlation-adjusted portfolio volatility
     *
     * @param positions The current portfolio positions
     * @return double The portfolio volatility
     */
    double calculatePortfolioVolatility(const std::vector<Position>& positions);

    /**
     * @brief Calculate the concentration risk for each asset class
     *
     * @param positions The current portfolio positions
     * @return std::unordered_map<std::string, double> Map of asset class to concentration percentage
     */
    std::unordered_map<std::string, double> calculateConcentrationRisk(
        const std::vector<Position>& positions);

    //! Configuration parameters for risk management
    RiskConfig m_config;

    //! Historical correlation matrix between assets
    std::unordered_map<std::pair<std::string, std::string>, double> m_correlationMatrix;

    //! Historical volatility for each asset
    std::unordered_map<std::string, double> m_volatilityMap;
};

} // namespace risk
} // namespace backtester

#endif // RISK_MANAGER_H
```

### Documentation Tools

The Live Strategy Backtester project uses the following documentation tools:

1. **Doxygen**: For generating API documentation from source code comments
2. **Markdown**: For user documentation, guides, and tutorials
3. **PlantUML**: For creating UML diagrams in documentation

### Documentation Generation

To generate documentation from code comments:

```bash
# Navigate to the project root
cd /path/to/Live_Strategy_Backtester

# Generate documentation using Doxygen
doxygen Doxyfile

# View the generated documentation
open docs/html/index.html
```

