# System Usage & Strategy Development Guide

## Quick Start

### Your First Backtest

Here's how to run a simple backtest in 5 steps:

```cpp
#include "core/Backtester.h"
#include "data/CSVDataSource.h"
#include "strategy/BasicStrategy.h"

int main() {
    // 1. Create backtester
    Backtester tester;
    
    // 2. Load data
    auto dataSource = std::make_shared<CSVDataSource>();
    dataSource->addFile("BTCUSDT", "data/BTCUSDT-1s-2025-07-13.csv");
    tester.setDataSource(dataSource);
    
    // 3. Create strategy
    auto strategy = std::make_shared<BasicStrategy>();
    strategy->setParameter("lookbackPeriod", 20);
    tester.setStrategy(strategy);
    
    // 4. Run backtest
    tester.run();
    
    // 5. View results
    tester.printResults();
    
    return 0;
}
```

## Strategy Development

### Basic Strategy Structure

Every strategy needs these 4 parts:

```cpp
class MyStrategy : public Strategy {
public:
    void initialize() override {
        // Setup indicators and parameters
    }
    
    void onBar(const BarData& bar) override {
        // Process new price data
        // Generate signals
        // Execute trades
    }
    
    void onTick(const TickData& tick) override {
        // Optional: handle tick data
    }
    
    void onOrderUpdate(const OrderUpdate& update) override {
        // Optional: handle order fills
    }
};
```

### Simple Moving Average Strategy

```cpp
class SimpleMAStrategy : public Strategy {
private:
    int shortPeriod = 10;
    int longPeriod = 30;
    std::vector<double> prices;
    
public:
    void initialize() override {
        prices.clear();
    }
    
    void onBar(const BarData& bar) override {
        prices.push_back(bar.close);
        
        // Keep only what we need
        if (prices.size() > longPeriod) {
            prices.erase(prices.begin());
        }
        
        // Need enough data
        if (prices.size() < longPeriod) return;
        
        // Calculate moving averages
        double shortMA = average(prices, shortPeriod);
        double longMA = average(prices, longPeriod);
        
        // Generate signals
        if (shortMA > longMA && getCurrentPosition() <= 0) {
            // Buy signal
            enterLong(1.0);
        }
        else if (shortMA < longMA && getCurrentPosition() >= 0) {
            // Sell signal
            enterShort(1.0);
        }
    }
    
private:
    double average(const std::vector<double>& data, int period) {
        double sum = 0;
        for (int i = data.size() - period; i < data.size(); i++) {
            sum += data[i];
        }
        return sum / period;
    }
};
```

## Development Process

### 1. Plan Your Strategy
- What pattern are you trying to catch?
- When do you buy/sell?
- How much risk do you take?

### 2. Code It Simple
- Start with basic logic
- Test one thing at a time
- Keep it readable

### 3. Test It
- Run backtests on historical data
- Check different time periods
- Look for problems

### 4. Improve It
- Add risk management
- Optimize parameters
- Fix issues you found

## Common Strategy Types

### Trend Following
```cpp
// Buy when price breaks above resistance
if (currentPrice > highestPrice(20)) {
    enterLong(1.0);
}
```

### Mean Reversion
```cpp
// Buy when price is too low
if (currentPrice < movingAverage(20) * 0.95) {
    enterLong(1.0);
}
```

### Breakout
```cpp
// Buy when volume confirms breakout
if (currentPrice > resistance && currentVolume > averageVolume(10)) {
    enterLong(1.0);
}
```

## Multi-Asset Strategy

```cpp
class MultiAssetStrategy : public Strategy {
private:
    std::map<std::string, double> lastPrices;
    
public:
    void onBar(const BarData& bar) override {
        // Track prices for each symbol
        lastPrices[bar.symbol] = bar.close;
        
        // Simple pairs trading example
        if (bar.symbol == "BTCUSDT") {
            double btcPrice = bar.close;
            double ethPrice = lastPrices["ETHUSDT"];
            
            // Trade based on price ratio
            double ratio = btcPrice / ethPrice;
            if (ratio > 15.0) {
                // BTC too expensive vs ETH
                enterShort("BTCUSDT", 0.5);
                enterLong("ETHUSDT", 0.5);
            }
        }
    }
};
```

## Risk Management

### Position Sizing
```cpp
double calculatePositionSize(double accountValue, double riskPercent) {
    return accountValue * (riskPercent / 100.0);
}

// Use 2% risk per trade
double size = calculatePositionSize(getAccountValue(), 2.0);
enterLong(size);
```

### Stop Loss
```cpp
void onBar(const BarData& bar) override {
    // Check for stop loss
    if (getCurrentPosition() > 0 && bar.close < entryPrice * 0.98) {
        // Lost 2%, exit
        closePosition();
    }
}
```

## Performance Analysis

### Key Metrics
- **Total Return**: How much profit/loss
- **Win Rate**: Percentage of winning trades
- **Max Drawdown**: Worst losing streak
- **Sharpe Ratio**: Risk-adjusted returns

### View Results
```cpp
// After backtest
auto results = tester.getResults();
std::cout << "Total Return: " << results.totalReturn << std::endl;
std::cout << "Win Rate: " << results.winRate << "%" << std::endl;
std::cout << "Max Drawdown: " << results.maxDrawdown << std::endl;
```

## Best Practices

### Keep It Simple
- Start with basic strategies
- Add complexity slowly
- Test each change

### Test Thoroughly
- Use different time periods
- Test bull and bear markets
- Check edge cases

### Manage Risk
- Never risk more than you can lose
- Use stop losses
- Diversify across assets

### Document Everything
- Comment your code
- Record your assumptions
- Note what works and what doesn't

## Common Problems

### Slow Backtests
- Use fewer indicators
- Process less data
- Optimize your code

### Unrealistic Results
- Include trading fees
- Account for slippage
- Use realistic position sizes

### Overfitting
- Test on different data
- Keep strategies simple
- Validate out-of-sample

## Getting Help

### Quick Checks
1. Are your data files correct?
2. Are your parameters reasonable?
3. Is your logic working as expected?

### Support
- Check the documentation
- Look at example strategies
- Contact support if needed

## Example: Complete RSI Strategy

```cpp
class RSIStrategy : public Strategy {
private:
    std::vector<double> prices;
    int period = 14;
    
public:
    void onBar(const BarData& bar) override {
        prices.push_back(bar.close);
        
        if (prices.size() > period + 1) {
            prices.erase(prices.begin());
        }
        
        if (prices.size() < period + 1) return;
        
        double rsi = calculateRSI();
        
        // Oversold - buy
        if (rsi < 30 && getCurrentPosition() <= 0) {
            enterLong(calculatePositionSize());
        }
        // Overbought - sell
        else if (rsi > 70 && getCurrentPosition() >= 0) {
            enterShort(calculatePositionSize());
        }
    }
    
private:
    double calculateRSI() {
        double gains = 0, losses = 0;
        
        for (int i = 1; i < prices.size(); i++) {
            double change = prices[i] - prices[i-1];
            if (change > 0) gains += change;
            else losses += -change;
        }
        
        if (losses == 0) return 100;
        
        double avgGain = gains / period;
        double avgLoss = losses / period;
        double rs = avgGain / avgLoss;
        
        return 100 - (100 / (1 + rs));
    }
    
    double calculatePositionSize() {
        return getAccountValue() * 0.05; // 5% of account
    }
};
```

## Next Steps

1. **Start Simple**: Try the moving average example
2. **Experiment**: Modify parameters and logic
3. **Test**: Run backtests on different data
4. **Learn**: Study what works and what doesn't
5. **Improve**: Add risk management and optimization

Remember: The best strategy is one that's simple, robust, and well-tested!
