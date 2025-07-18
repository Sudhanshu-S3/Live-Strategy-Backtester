# Frequently Asked Questions

## General Questions

### What is the Live Strategy Backtester?

The Live Strategy Backtester is a high-performance C++ framework for developing, testing, and optimizing trading strategies across multiple asset classes with real-time capabilities.

### What programming languages are supported?

The core framework is written in C++, but strategies can be developed in:

- C++ (native)
- Python (via bindings)
- R (via interface module)

### Which asset classes are supported?

The backtester supports multiple asset classes:

- Equities
- Futures
- Options
- Cryptocurrencies
- Forex
- Fixed Income

## Installation & Setup

### What are the system requirements?

- Operating System: Windows 10+, Linux, macOS
- CPU: Multi-core processor recommended
- RAM: Minimum 8GB (16GB+ recommended for large datasets)
- Storage: SSD recommended for faster data access

### How do I install the backtester?

Follow the instructions in the [Getting Started](../../README.md#getting-started) section of the README.

### Can I use my own data sources?

Yes, the system supports custom data sources through the `IDataSource` interface. Implement this interface to integrate your own data provider.

## Usage Questions

### How do I create a new strategy?

See the [Strategy Development Guide](strategy_development_guide.md) for detailed instructions.

### Can I backtest multiple strategies simultaneously?

Yes, use the `MultiStrategyBacktester` class to run multiple strategies in parallel.

### How can I optimize strategy parameters?

Use the built-in parameter optimization framework:

```cpp
StrategyOptimizer optimizer;
optimizer.addParameter("lookbackPeriod", 10, 100, 5);
optimizer.addParameter("threshold", 0.5, 3.0, 0.1);
optimizer.optimize(myStrategy, dataSource);
```
