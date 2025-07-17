# Design Patterns

## Overview

The Live Strategy Backtester implements several design patterns to ensure code quality, maintainability, and extensibility. Understanding these patterns will help developers contribute to the project effectively.

## Core Design Patterns

### Factory Pattern

Used to create different implementations of data sources, strategies, and execution engines:

```cpp
class DataSourceFactory {
public:
    static std::shared_ptr<DataSource> createDataSource(const std::string& type,
                                                      const std::string& source);
};

// Usage
auto dataSource = DataSourceFactory::createDataSource("csv", "data/BTCUSDT-1s-2025-07-13.csv");
```

### Strategy Pattern

Used to define interchangeable trading algorithms:

```cpp
class Strategy {
public:
    virtual Signal generateSignal(const MarketData& data) = 0;
    virtual ~Strategy() = default;
};

class MovingAverageCrossover : public Strategy {
public:
    Signal generateSignal(const MarketData& data) override;
private:
    // Strategy-specific implementation
};
```

### Observer Pattern

Used for event handling and notifications:

```cpp
// Example of order execution events
class OrderExecutionObserver {
public:
    virtual void onOrderFilled(const Order& order) = 0;
    virtual void onOrderRejected(const Order& order, const std::string& reason) = 0;
    virtual ~OrderExecutionObserver() = default;
};
```

### Dependency Injection

Used to provide components with their dependencies:

```cpp
class Backtester {
public:
    void setDataSource(std::shared_ptr<DataSource> dataSource) {
        m_dataSource = dataSource;
    }

    void setStrategy(std::shared_ptr<Strategy> strategy) {
        m_strategy = strategy;
    }

    void setRiskManager(std::shared_ptr<RiskManager> riskManager) {
        m_riskManager = riskManager;
    }

private:
    std::shared_ptr<DataSource> m_dataSource;
    std::shared_ptr<Strategy> m_strategy;
    std::shared_ptr<RiskManager> m_riskManager;
};
```

### Command Pattern

Used for order execution and transaction handling:

```cpp
class OrderCommand {
public:
    virtual void execute() = 0;
    virtual void cancel() = 0;
    virtual ~OrderCommand() = default;
};

class MarketOrderCommand : public OrderCommand {
public:
    MarketOrderCommand(const std::string& symbol, double quantity, bool isBuy);
    void execute() override;
    void cancel() override;
private:
    // Implementation details
};
```

## Architectural Patterns

### Model-View-Controller (MVC)

- **Model**: Market data, orders, positions
- **View**: Performance dashboards, equity curves, position visualizations
- **Controller**: Backtester core, execution engine

### Pipeline Processing

Market data flows through a series of processing stages:

1. Data loading and normalization
2. Signal generation
3. Order creation
4. Risk validation
5. Execution simulation
6. Performance calculation

## Best Practices

- **Interface Segregation**: Components interact through small, focused interfaces
- **Composition over Inheritance**: Complex components built through composition
- **RAII (Resource Acquisition Is Initialization)**: For proper resource management
- **Thread Safety**: Critical components designed for thread-safe operation
