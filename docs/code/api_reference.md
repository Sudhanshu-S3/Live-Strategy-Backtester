# API Reference for Strategy Development

## Overview

This document provides a comprehensive reference for the API components used in strategy development with the Live Strategy Backtester. Understanding these components is essential for creating custom trading strategies.

## Core Components

### Strategy Base Class

The foundation for all trading strategy implementations.

```cpp
namespace backtester {
namespace strategy {

class Strategy {
public:
    // Lifecycle methods
    virtual void initialize(const StrategyConfig& config);
    virtual void onStart();
    virtual void onStop();

    // Event handlers
    virtual void onBar(const Bar& bar);
    virtual void onTrade(const Trade& trade);
    virtual void onQuote(const Quote& quote);
    virtual void onOrderBook(const OrderBook& orderBook);
    virtual void onOrderUpdate(const Order& order);
    virtual void onPositionUpdate(const Position& position);
    virtual void onMarketState(const MarketState& state);
    virtual void onTimer(const TimerEvent& timer);

    // Signal generation
    virtual double generateSignal(const MarketData& data);

    // Order management
    Order createOrder(const std::string& symbol, OrderType type, OrderSide side,
                     double quantity, double price = 0.0);
    bool modifyOrder(const std::string& orderId, double newQuantity, double newPrice);
    bool cancelOrder(const std::string& orderId);

    // Position information
    Position getPosition(const std::string& symbol);
    std::vector<Position> getAllPositions();

    // Market data access
    MarketData getMarketData(const std::string& symbol,
                            DataFrequency frequency = DataFrequency::MINUTE,
                            int lookback = 100);

    // Performance tracking
    PerformanceMetrics getPerformance();

    // Utility methods
    void setParameter(const std::string& name, double value);
    double getParameter(const std::string& name) const;
    void log(LogLevel level, const std::string& message);
};

} // namespace strategy
} // namespace backtester
```

### Example: Simple Moving Average Crossover Strategy

```cpp
class MovingAverageCrossover : public Strategy {
public:
    void initialize(const StrategyConfig& config) override {
        m_fastPeriod = config.getParameter("fastPeriod", 20);
        m_slowPeriod = config.getParameter("slowPeriod", 50);
        m_symbol = config.getParameter("symbol", "BTCUSDT");
    }

    void onBar(const Bar& bar) override {
        if (bar.symbol != m_symbol) return;

        // Get historical data
        auto data = getMarketData(m_symbol, DataFrequency::MINUTE, m_slowPeriod + 10);

        // Calculate moving averages
        double fastMA = calculateSMA(data.closes, m_fastPeriod);
        double slowMA = calculateSMA(data.closes, m_slowPeriod);

        // Current position
        auto position = getPosition(m_symbol);
        double currentPos = position.quantity;

        // Generate signals
        if (fastMA > slowMA && currentPos <= 0) {
            // Bullish crossover
            double qty = calculatePositionSize(data);
            createOrder(m_symbol, OrderType::MARKET, OrderSide::BUY, qty);
            log(LogLevel::INFO, "Buy signal: Fast MA crossed above Slow MA");
        }
        else if (fastMA < slowMA && currentPos > 0) {
            // Bearish crossover
            createOrder(m_symbol, OrderType::MARKET, OrderSide::SELL, currentPos);
            log(LogLevel::INFO, "Sell signal: Fast MA crossed below Slow MA");
        }
    }

private:
    double calculateSMA(const std::vector<double>& prices, int period) {
        if (prices.size() < period) return 0;

        double sum = 0;
        for (int i = prices.size() - period; i < prices.size(); i++) {
            sum += prices[i];
        }
        return sum / period;
    }

    double calculatePositionSize(const MarketData& data) {
        // Simple position sizing based on volatility
        double atr = calculateATR(data.highs, data.lows, data.closes, 14);
        double riskAmount = getParameter("riskPerTrade", 0.01) * getPerformance().equity;
        return riskAmount / atr;
    }

    double calculateATR(const std::vector<double>& highs,
                       const std::vector<double>& lows,
                       const std::vector<double>& closes,
                       int period) {
        // ATR calculation implementation
        // ...
        return 0.01 * closes.back(); // Simplified for this example
    }

    int m_fastPeriod;
    int m_slowPeriod;
    std::string m_symbol;
};
```

## Data Structures

### MarketData

Container for historical market data.

```cpp
struct MarketData {
    std::string symbol;
    DataFrequency frequency;
    std::vector<datetime_t> timestamps;
    std::vector<double> opens;
    std::vector<double> highs;
    std::vector<double> lows;
    std::vector<double> closes;
    std::vector<double> volumes;

    // Utility methods
    size_t size() const { return timestamps.size(); }

    // Get a slice of data
    MarketData getSlice(int startIndex, int endIndex) const;

    // Resample to a different frequency
    MarketData resample(DataFrequency newFreq) const;
};
```

### Bar

Represents a single OHLCV bar.

```cpp
struct Bar {
    std::string symbol;
    datetime_t timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume;
    DataFrequency frequency;
};
```

### Trade

Represents a single trade execution.

```cpp
struct Trade {
    std::string symbol;
    datetime_t timestamp;
    double price;
    double quantity;
    TradeDirection direction; // BUY or SELL
    std::string tradeId;
};
```

### Quote

Represents a bid and ask quote.

```cpp
struct Quote {
    std::string symbol;
    datetime_t timestamp;
    double bidPrice;
    double bidSize;
    double askPrice;
    double askSize;
};
```

### OrderBook

Represents a full order book snapshot.

```cpp
struct OrderBookLevel {
    double price;
    double quantity;
    int orderCount;
};

struct OrderBook {
    std::string symbol;
    datetime_t timestamp;
    std::vector<OrderBookLevel> bids; // Sorted by price (descending)
    std::vector<OrderBookLevel> asks; // Sorted by price (ascending)

    // Helper methods
    double getBestBid() const { return bids.empty() ? 0 : bids[0].price; }
    double getBestAsk() const { return asks.empty() ? 0 : asks[0].price; }
    double getMidPrice() const { return (getBestBid() + getBestAsk()) / 2.0; }
    double getSpread() const { return getBestAsk() - getBestBid(); }
    double getImbalance() const;
};
```

### Order

Represents a trading order.

```cpp
enum class OrderType {
    MARKET,
    LIMIT,
    STOP,
    STOP_LIMIT,
    TRAILING_STOP,
    IOC, // Immediate or Cancel
    FOK  // Fill or Kill
};

enum class OrderSide {
    BUY,
    SELL
};

enum class OrderStatus {
    PENDING,
    ACTIVE,
    PARTIALLY_FILLED,
    FILLED,
    CANCELED,
    REJECTED,
    EXPIRED
};

struct Order {
    std::string orderId;
    std::string symbol;
    OrderType type;
    OrderSide side;
    OrderStatus status;
    double quantity;
    double filledQuantity;
    double price;
    double stopPrice;
    datetime_t createdTime;
    datetime_t updatedTime;

    // For tracking partial fills
    std::vector<Trade> fills;

    // Helper methods
    bool isActive() const {
        return status == OrderStatus::ACTIVE ||
               status == OrderStatus::PARTIALLY_FILLED;
    }

    double getRemainingQuantity() const {
        return quantity - filledQuantity;
    }

    double getAverageFilledPrice() const;
};
```

### Position

Represents a trading position.

```cpp
struct Position {
    std::string symbol;
    double quantity;
    double averageEntryPrice;
    datetime_t entryTime;
    double unrealizedPnL;
    double realizedPnL;

    // Helper methods
    bool isLong() const { return quantity > 0; }
    bool isShort() const { return quantity < 0; }
    bool isFlat() const { return quantity == 0; }
    double getMarketValue(double currentPrice) const {
        return quantity * currentPrice;
    }
    double getTotalPnL(double currentPrice) const {
        return realizedPnL + (currentPrice - averageEntryPrice) * quantity;
    }
};
```

### PerformanceMetrics

Represents performance metrics for a strategy.

```cpp
struct PerformanceMetrics {
    double equity;              // Current equity value
    double startingEquity;      // Initial equity value
    double totalPnL;            // Total profit and loss
    double totalReturnPct;      // Total return percentage
    double sharpeRatio;         // Sharpe ratio
    double sortinoRatio;        // Sortino ratio
    double maxDrawdownPct;      // Maximum drawdown percentage
    double maxDrawdownAmount;   // Maximum drawdown amount
    double winRate;             // Win rate
    double profitFactor;        // Profit factor
    int totalTrades;            // Total number of trades
    int winningTrades;          // Number of winning trades
    int losingTrades;           // Number of losing trades
    double averageWin;          // Average win amount
    double averageLoss;         // Average loss amount
    double averageTradePnL;     // Average trade P&L
    double averageHoldingTime;  // Average holding time

    // Equity curve
    std::vector<datetime_t> timestamps;
    std::vector<double> equityCurve;
    std::vector<double> drawdownCurve;

    // Monthly returns
    std::map<std::string, double> monthlyReturns;

    // Trade list
    std::vector<TradeRecord> trades;
};
```

## Utility Classes

### StrategyConfig

Configuration for a strategy.

```cpp
class StrategyConfig {
public:
    // Set and get parameters
    void setParameter(const std::string& name, double value);
    void setParameter(const std::string& name, const std::string& value);
    void setParameter(const std::string& name, bool value);

    double getParameter(const std::string& name, double defaultValue = 0.0) const;
    std::string getParameter(const std::string& name, const std::string& defaultValue = "") const;
    bool getParameter(const std::string& name, bool defaultValue = false) const;

    // Load and save configuration
    void loadFromFile(const std::string& filename);
    void saveToFile(const std::string& filename) const;

    // Get all parameter names
    std::vector<std::string> getParameterNames() const;
};
```

### Indicators

Technical indicators for strategy development.

```cpp
namespace indicators {

// Moving averages
std::vector<double> SMA(const std::vector<double>& prices, int period);
std::vector<double> EMA(const std::vector<double>& prices, int period);
std::vector<double> WMA(const std::vector<double>& prices, int period);
std::vector<double> HullMA(const std::vector<double>& prices, int period);

// Oscillators
std::vector<double> RSI(const std::vector<double>& prices, int period);
std::vector<double> MACD(const std::vector<double>& prices,
                        int fastPeriod, int slowPeriod, int signalPeriod,
                        std::vector<double>& signalLine,
                        std::vector<double>& histogram);
std::vector<double> Stochastic(const std::vector<double>& highs,
                              const std::vector<double>& lows,
                              const std::vector<double>& closes,
                              int period, int kSmooth, int dSmooth,
                              std::vector<double>& stochD);

// Volatility indicators
std::vector<double> BollingerBands(const std::vector<double>& prices,
                                  int period, double stdDevMultiplier,
                                  std::vector<double>& upperBand,
                                  std::vector<double>& lowerBand);
std::vector<double> ATR(const std::vector<double>& highs,
                       const std::vector<double>& lows,
                       const std::vector<double>& closes,
                       int period);
std::vector<double> Chandelier(const std::vector<double>& highs,
                              const std::vector<double>& lows,
                              const std::vector<double>& closes,
                              int period, double multiplier,
                              bool isLong);

// Volume indicators
std::vector<double> OBV(const std::vector<double>& closes,
                       const std::vector<double>& volumes);
std::vector<double> VWAP(const std::vector<double>& highs,
                        const std::vector<double>& lows,
                        const std::vector<double>& closes,
                        const std::vector<double>& volumes);
std::vector<double> AccumulationDistribution(const std::vector<double>& highs,
                                           const std::vector<double>& lows,
                                           const std::vector<double>& closes,
                                           const std::vector<double>& volumes);

// Pattern recognition
std::vector<int> CandlePatterns(const std::vector<double>& opens,
                               const std::vector<double>& highs,
                               const std::vector<double>& lows,
                               const std::vector<double>& closes);
std::vector<int> SupportResistance(const std::vector<double>& highs,
                                  const std::vector<double>& lows,
                                  int lookbackPeriod);

} // namespace indicators
```

### Optimization

Classes for parameter optimization.

```cpp
namespace optimization {

class Optimizer {
public:
    // Setup optimization parameters
    void addParameter(const std::string& name, double min, double max, double step);
    void setObjectiveFunction(const std::function<double(const StrategyConfig&)>& func);
    void setMaximize(bool maximize = true);

    // Run optimization
    virtual StrategyConfig optimize() = 0;

    // Get results
    std::vector<StrategyConfig> getTopConfigurations(int n = 10) const;

protected:
    struct Parameter {
        std::string name;
        double min;
        double max;
        double step;
    };

    std::vector<Parameter> m_parameters;
    std::function<double(const StrategyConfig&)> m_objectiveFunction;
    bool m_maximize;
    std::vector<std::pair<double, StrategyConfig>> m_results;
};

// Grid search optimizer
class GridSearchOptimizer : public Optimizer {
public:
    StrategyConfig optimize() override;
};

// Random search optimizer
class RandomSearchOptimizer : public Optimizer {
public:
    RandomSearchOptimizer(int numSamples = 1000);
    StrategyConfig optimize() override;

private:
    int m_numSamples;
};

// Genetic algorithm optimizer
class GeneticOptimizer : public Optimizer {
public:
    GeneticOptimizer(int populationSize = 50, int generations = 20);
    StrategyConfig optimize() override;

    // Genetic algorithm specific settings
    void setCrossoverRate(double rate);
    void setMutationRate(double rate);
    void setElitism(int eliteCount);

private:
    int m_populationSize;
    int m_generations;
    double m_crossoverRate;
    double m_mutationRate;
    int m_eliteCount;
};

// Bayesian optimization
class BayesianOptimizer : public Optimizer {
public:
    BayesianOptimizer(int iterations = 100);
    StrategyConfig optimize() override;

private:
    int m_iterations;
};

} // namespace optimization
```

### Risk Management

Classes for risk management.

```cpp
namespace risk {

class RiskManager {
public:
    RiskManager(const RiskConfig& config);

    // Position sizing
    double calculatePositionSize(const std::string& symbol,
                                double price,
                                double stopPrice);

    // Risk checks
    bool checkPositionRisk(const std::string& symbol, double quantity, double price);
    bool checkPortfolioRisk(const std::vector<Position>& positions);

    // Risk metrics
    double calculateValueAtRisk(const std::vector<Position>& positions,
                               double confidenceLevel = 0.95);
    double calculateDrawdown(const std::vector<double>& equityCurve);
    double calculatePortfolioVolatility(const std::vector<Position>& positions);

    // Risk limits
    void setMaxDrawdown(double maxDrawdownPct);
    void setMaxPositionSize(double maxPositionSizePct);
    void setMaxConcentration(double maxConcentrationPct);
};

struct RiskConfig {
    double riskPerTrade;       // Risk per trade as percentage of equity
    double maxDrawdown;        // Maximum allowed drawdown percentage
    double maxPositionSize;    // Maximum position size as percentage of equity
    double maxConcentration;   // Maximum concentration in a single asset
    double maxCorrelation;     // Maximum correlation between portfolio assets
    double confidenceLevel;    // Confidence level for Value at Risk (VaR)
    int varTimeHorizon;        // Time horizon for VaR calculation in days
};

} // namespace risk
```

## Event System

The event system for handling market events.

```cpp
namespace event {

enum class EventType {
    BAR,
    TRADE,
    QUOTE,
    ORDER_BOOK,
    ORDER_UPDATE,
    POSITION_UPDATE,
    MARKET_STATE,
    TIMER,
    CUSTOM
};

class Event {
public:
    virtual EventType getType() const = 0;
    virtual datetime_t getTimestamp() const = 0;
    virtual ~Event() = default;
};

// Example of a specific event
class BarEvent : public Event {
public:
    BarEvent(const Bar& bar) : m_bar(bar) {}

    EventType getType() const override { return EventType::BAR; }
    datetime_t getTimestamp() const override { return m_bar.timestamp; }
    const Bar& getBar() const { return m_bar; }

private:
    Bar m_bar;
};

class EventDispatcher {
public:
    using EventHandler = std::function<void(const Event&)>;

    // Register handlers for event types
    void registerHandler(EventType type, EventHandler handler);

    // Dispatch an event to registered handlers
    void dispatch(const Event& event);

private:
    std::unordered_map<EventType, std::vector<EventHandler>> m_handlers;
};

} // namespace event
```

## Working with the API

### Creating a Custom Strategy

1. Inherit from the `Strategy` base class
2. Override the necessary event handlers
3. Implement your trading logic

### Strategy Lifecycle

1. `initialize`: Called once when the strategy is loaded
2. `onStart`: Called when the backtesting or live trading session starts
3. Event handlers: Called when corresponding market events occur
4. `onStop`: Called when the backtesting or live trading session ends

### Data Access Patterns

```cpp
// Get the latest N bars of data
auto data = getMarketData("BTCUSDT", DataFrequency::MINUTE, 100);

// Access specific data points
double lastClose = data.closes.back();
double previousClose = data.closes[data.closes.size() - 2];

// Calculate indicators
auto sma20 = indicators::SMA(data.closes, 20);
auto rsi14 = indicators::RSI(data.closes, 14);

// Access multiple timeframes
auto hourlyData = getMarketData("BTCUSDT", DataFrequency::HOUR, 24);
auto dailyData = getMarketData("BTCUSDT", DataFrequency::DAY, 30);
```

### Order Management Patterns

```cpp
// Market order
createOrder("BTCUSDT", OrderType::MARKET, OrderSide::BUY, 1.0);

// Limit order
createOrder("BTCUSDT", OrderType::LIMIT, OrderSide::BUY, 1.0, 50000.0);

// Stop order
auto order = createOrder("BTCUSDT", OrderType::STOP, OrderSide::SELL, 1.0, 0.0);
order.stopPrice = 45000.0;

// Cancel an order
cancelOrder(orderId);

// Modify an order
modifyOrder(orderId, 2.0, 51000.0);
```

### Position Management Patterns

```cpp
// Get current position
auto position = getPosition("BTCUSDT");

// Check position status
if (position.isLong()) {
    // Handle long position
} else if (position.isShort()) {
    // Handle short position
} else {
    // No position
}

// Close a position
if (!position.isFlat()) {
    createOrder("BTCUSDT", OrderType::MARKET,
               position.isLong() ? OrderSide::SELL : OrderSide::BUY,
               std::abs(position.quantity));
}
```

### Risk Management Patterns

```cpp
// Create a risk manager with config
RiskConfig riskConfig;
riskConfig.riskPerTrade = 0.01; // 1% risk per trade
riskConfig.maxDrawdown = 0.2;   // 20% max drawdown
RiskManager riskManager(riskConfig);

// Calculate position size based on stop loss
double price = data.closes.back();
double stopPrice = price * 0.95; // 5% stop loss
double positionSize = riskManager.calculatePositionSize("BTCUSDT", price, stopPrice);

// Check if a new position would exceed risk limits
if (riskManager.checkPositionRisk("BTCUSDT", positionSize, price)) {
    createOrder("BTCUSDT", OrderType::MARKET, OrderSide::BUY, positionSize);
}
```

### Performance Analysis Patterns

````cpp
// Get current performance metrics
auto metrics = getPerformance();

// Log key metrics
log(LogLevel::INFO, "Current equity: " + std::to_string(metrics.equity));
log(LogLevel::INFO, "Total return# API Reference for Strategy Development

## Overview

This document provides a comprehensive reference for the API components used in strategy development with the Live Strategy Backtester. Understanding these components is essential for creating custom trading strategies.

## Core Components

### Strategy Base Class

The foundation for all trading strategy implementations.

```cpp
namespace backtester {
namespace strategy {

class Strategy {
public:
    // Lifecycle methods
    virtual void initialize(const StrategyConfig& config);
    virtual void onStart();
    virtual void onStop();

    // Event handlers
    virtual void onBar(const Bar& bar);
    virtual void onTrade(const Trade& trade);
    virtual void onQuote(const Quote& quote);
    virtual void onOrderBook(const OrderBook& orderBook);
    virtual void onOrderUpdate(const Order& order);
    virtual void onPositionUpdate(const Position& position);
    virtual void onMarketState(const MarketState& state);
    virtual void onTimer(const TimerEvent& timer);

    // Signal generation
    virtual double generateSignal(const MarketData& data);

    // Order management
    Order createOrder(const std::string& symbol, OrderType type, OrderSide side,
                     double quantity, double price = 0.0);
    bool modifyOrder(const std::string& orderId, double newQuantity, double newPrice);
    bool cancelOrder(const std::string& orderId);

    // Position information
    Position getPosition(const std::string& symbol);
    std::vector<Position> getAllPositions();

    // Market data access
    MarketData getMarketData(const std::string& symbol,
                            DataFrequency frequency = DataFrequency::MINUTE,
                            int lookback = 100);

    // Performance tracking
    PerformanceMetrics getPerformance();

    // Utility methods
    void setParameter(const std::string& name, double value);
    double getParameter(const std::string& name) const;
    void log(LogLevel level, const std::string& message);
};

} // namespace strategy
} // namespace backtester
````

### Example: Simple Moving Average Crossover Strategy

```cpp
class MovingAverageCrossover : public Strategy {
public:
    void initialize(const StrategyConfig& config) override {
        m_fastPeriod = config.getParameter("fastPeriod", 20);
        m_slowPeriod = config.getParameter("slowPeriod", 50);
        m_symbol = config.getParameter("symbol", "BTCUSDT");
    }

    void onBar(const Bar& bar) override {
        if (bar.symbol != m_symbol) return;

        // Get historical data
        auto data = getMarketData(m_symbol, DataFrequency::MINUTE, m_slowPeriod + 10);

        // Calculate moving averages
        double fastMA = calculateSMA(data.closes, m_fastPeriod);
        double slowMA = calculateSMA(data.closes, m_slowPeriod);

        // Current position
        auto position = getPosition(m_symbol);
        double currentPos = position.quantity;

        // Generate signals
        if (fastMA > slowMA && currentPos <= 0) {
            // Bullish crossover
            double qty = calculatePositionSize(data);
            createOrder(m_symbol, OrderType::MARKET, OrderSide::BUY, qty);
            log(LogLevel::INFO, "Buy signal: Fast MA crossed above Slow MA");
        }
        else if (fastMA < slowMA && currentPos > 0) {
            // Bearish crossover
            createOrder(m_symbol, OrderType::MARKET, OrderSide::SELL, currentPos);
            log(LogLevel::INFO, "Sell signal: Fast MA crossed below Slow MA");
        }
    }

private:
    double calculateSMA(const std::vector<double>& prices, int period) {
        if (prices.size() < period) return 0;

        double sum = 0;
        for (int i = prices.size() - period; i < prices.size(); i++) {
            sum += prices[i];
        }
        return sum / period;
    }

    double calculatePositionSize(const MarketData& data) {
        // Simple position sizing based on volatility
        double atr = calculateATR(data.highs, data.lows, data.closes, 14);
        double riskAmount = getParameter("riskPerTrade", 0.01) * getPerformance().equity;
        return riskAmount / atr;
    }

    double calculateATR(const std::vector<double>& highs,
                       const std::vector<double>& lows,
                       const std::vector<double>& closes,
                       int period) {
        // ATR calculation implementation
        // ...
        return 0.01 * closes.back(); // Simplified for this example
    }

    int m_fastPeriod;
    int m_slowPeriod;
    std::string m_symbol;
};
```

## Data Structures

### MarketData

Container for historical market data.

```cpp
struct MarketData {
    std::string symbol;
    DataFrequency frequency;
    std::vector<datetime_t> timestamps;
    std::vector<double> opens;
    std::vector<double> highs;
    std::vector<double> lows;
    std::vector<double> closes;
    std::vector<double> volumes;

    // Utility methods
    size_t size() const { return timestamps.size(); }

    // Get a slice of data
    MarketData getSlice(int startIndex, int endIndex) const;

    // Resample to a different frequency
    MarketData resample(DataFrequency newFreq) const;
};
```

### Bar

Represents a single OHLCV bar.

```cpp
struct Bar {
    std::string symbol;
    datetime_t timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume;
    DataFrequency frequency;
};
```

### Trade

Represents a single trade execution.

```cpp
struct Trade {
    std::string symbol;
    datetime_t timestamp;
    double price;
    double quantity;
    TradeDirection direction; // BUY or SELL
    std::string tradeId;
};
```

### Quote

Represents a bid and ask quote.

```cpp
struct Quote {
    std::string symbol;
    datetime_t timestamp;
    double bidPrice;
    double bidSize;
    double askPrice;
    double askSize;
};
```

### OrderBook

Represents a full order book snapshot.

```cpp
struct OrderBookLevel {
    double price;
    double quantity;
    int orderCount;
};

struct OrderBook {
    std::string symbol;
    datetime_t timestamp;
    std::vector<OrderBookLevel> bids; // Sorted by price (descending)
    std::vector<OrderBookLevel> asks; // Sorted by price (ascending)

    // Helper methods
    double getBestBid() const { return bids.empty() ? 0 : bids[0].price; }
    double getBestAsk() const { return asks.empty() ? 0 : asks[0].price; }
    double getMidPrice() const { return (getBestBid() + getBestAsk()) / 2.0; }
    double getSpread() const { return getBestAsk() - getBestBid(); }
    double getImbalance() const;
};
```

### Order

Represents a trading order.

```cpp
enum class OrderType {
    MARKET,
    LIMIT,
    STOP,
    STOP_LIMIT,
    TRAILING_STOP,
    IOC, // Immediate or Cancel
    FOK  // Fill or Kill
};

enum class OrderSide {
    BUY,
    SELL
};

enum class OrderStatus {
    PENDING,
    ACTIVE,
    PARTIALLY_FILLED,
    FILLED,
    CANCELED,
    REJECTED,
    EXPIRED
};

struct Order {
    std::string orderId;
    std::string symbol;
    OrderType type;
    OrderSide side;
    OrderStatus status;
    double quantity;
    double filledQuantity;
    double price;
    double stopPrice;
    datetime_t createdTime;
    datetime_t updatedTime;

    // For tracking partial fills
    std::vector<Trade> fills;

    // Helper methods
    bool isActive() const {
        return status == OrderStatus::ACTIVE ||
               status == OrderStatus::PARTIALLY_FILLED;
    }

    double getRemainingQuantity() const {
        return quantity - filledQuantity;
    }

    double getAverageFilledPrice() const;
};
```

### Position

Represents a trading position.

```cpp
struct Position {
    std::string symbol;
    double quantity;
    double averageEntryPrice;
    datetime_t entryTime;
    double unrealizedPnL;
    double realizedPnL;

    // Helper methods
    bool isLong() const { return quantity > 0; }
    bool isShort() const { return quantity < 0; }
    bool isFlat() const { return quantity == 0; }
    double getMarketValue(double currentPrice) const {
        return quantity * currentPrice;
    }
    double getTotalPnL(double currentPrice) const {
        return realizedPnL + (currentPrice - averageEntryPrice) * quantity;
    }
};
```

### PerformanceMetrics

Represents performance metrics for a strategy.

```cpp
struct PerformanceMetrics {
    double equity;              // Current equity value
    double startingEquity;      // Initial equity value
    double totalPnL;            // Total profit and loss
    double totalReturnPct;      // Total return percentage
    double sharpeRatio;         // Sharpe ratio
    double sortinoRatio;        // Sortino ratio
    double maxDrawdownPct;      // Maximum drawdown percentage
    double maxDrawdownAmount;   // Maximum drawdown amount
    double winRate;             // Win rate
    double profitFactor;        // Profit factor
    int totalTrades;            // Total number of trades
    int winningTrades;          // Number of winning trades
    int losingTrades;           // Number of losing trades
    double averageWin;          // Average win amount
    double averageLoss;         // Average loss amount
    double averageTradePnL;     // Average trade P&L
    double averageHoldingTime;  // Average holding time

    // Equity curve
    std::vector<datetime_t> timestamps;
    std::vector<double> equityCurve;
    std::vector<double> drawdownCurve;

    // Monthly returns
    std::map<std::string, double> monthlyReturns;

    // Trade list
    std::vector<TradeRecord> trades;
};
```

## Utility Classes

### StrategyConfig

Configuration for a strategy.

```cpp
class StrategyConfig {
public:
    // Set and get parameters
    void setParameter(const std::string& name, double value);
    void setParameter(const std::string& name, const std::string& value);
    void setParameter(const std::string& name, bool value);

    double getParameter(const std::string& name, double defaultValue = 0.0) const;
    std::string getParameter(const std::string& name, const std::string& defaultValue = "") const;
    bool getParameter(const std::string& name, bool defaultValue = false) const;

    // Load and save configuration
    void loadFromFile(const std::string& filename);
    void saveToFile(const std::string& filename) const;

    // Get all parameter names
    std::vector<std::string> getParameterNames() const;
};
```

### Indicators

Technical indicators for strategy development.

```cpp
namespace indicators {

// Moving averages
std::vector<double> SMA(const std::vector<double>& prices, int period);
std::vector<double> EMA(const std::vector<double>& prices, int period);
std::vector<double> WMA(const std::vector<double>& prices, int period);
std::vector<double> HullMA(const std::vector<double>& prices, int period);

// Oscillators
std::vector<double> RSI(const std::vector<double>& prices, int period);
std::vector<double> MACD(const std::vector<double>& prices,
                        int fastPeriod, int slowPeriod, int signalPeriod,
                        std::vector<double>& signalLine,
                        std::vector<double>& histogram);
std::vector<double> Stochastic(const std::vector<double>& highs,
                              const std::vector<double>& lows,
                              const std::vector<double>& closes,
                              int period, int kSmooth, int dSmooth,
                              std::vector<double>& stochD);

// Volatility indicators
std::vector<double> BollingerBands(const std::vector<double>& prices,
                                  int period, double stdDevMultiplier,
                                  std::vector<double>& upperBand,
                                  std::vector<double>& lowerBand);
std::vector<double> ATR(const std::vector<double>& highs,
                       const std::vector<double>& lows,
                       const std::vector<double>& closes,
                       int period);
std::vector<double> Chandelier(const std::vector<double>& highs,
                              const std::vector<double>& lows,
                              const std::vector<double>& closes,
                              int period, double multiplier,
                              bool isLong);

// Volume indicators
std::vector<double> OBV(const std::vector<double>& closes,
                       const std::vector<double>& volumes);
std::vector<double> VWAP(const std::vector<double>& highs,
                        const std::vector<double>& lows,
                        const std::vector<double>& closes,
                        const std::vector<double>& volumes);
std::vector<double> AccumulationDistribution(const std::vector<double>& highs,
                                           const std::vector<double>& lows,
                                           const std::vector<double>& closes,
                                           const std::vector<double>& volumes);

// Pattern recognition
std::vector<int> CandlePatterns(const std::vector<double>& opens,
                               const std::vector<double>& highs,
                               const std::vector<double>& lows,
                               const std::vector<double>& closes);
std::vector<int> SupportResistance(const std::vector<double>& highs,
                                  const std::vector<double>& lows,
                                  int lookbackPeriod);

} // namespace indicators
```

### Optimization

Classes for parameter optimization.

```cpp
namespace optimization {

class Optimizer {
public:
    // Setup optimization parameters
    void addParameter(const std::string& name, double min, double max, double step);
    void setObjectiveFunction(const std::function<double(const StrategyConfig&)>& func);
    void setMaximize(bool maximize = true);

    // Run optimization
    virtual StrategyConfig optimize() = 0;

    // Get results
    std::vector<StrategyConfig> getTopConfigurations(int n = 10) const;

protected:
    struct Parameter {
        std::string name;
        double min;
        double max;
        double step;
    };

    std::vector<Parameter> m_parameters;
    std::function<double(const StrategyConfig&)> m_objectiveFunction;
    bool m_maximize;
    std::vector<std::pair<double, StrategyConfig>> m_results;
};

// Grid search optimizer
class GridSearchOptimizer : public Optimizer {
public:
    StrategyConfig optimize() override;
};

// Random search optimizer
class RandomSearchOptimizer : public Optimizer {
public:
    RandomSearchOptimizer(int numSamples = 1000);
    StrategyConfig optimize() override;

private:
    int m_numSamples;
};

// Genetic algorithm optimizer
class GeneticOptimizer : public Optimizer {
public:
    GeneticOptimizer(int populationSize = 50, int generations = 20);
    StrategyConfig optimize() override;

    // Genetic algorithm specific settings
    void setCrossoverRate(double rate);
    void setMutationRate(double rate);
    void setElitism(int eliteCount);

private:
    int m_populationSize;
    int m_generations;
    double m_crossoverRate;
    double m_mutationRate;
    int m_eliteCount;
};

// Bayesian optimization
class BayesianOptimizer : public Optimizer {
public:
    BayesianOptimizer(int iterations = 100);
    StrategyConfig optimize() override;

private:
    int m_iterations;
};

} // namespace optimization
```

### Risk Management

Classes for risk management.

```cpp
namespace risk {

class RiskManager {
public:
    RiskManager(const RiskConfig& config);

    // Position sizing
    double calculatePositionSize(const std::string& symbol,
                                double price,
                                double stopPrice);

    // Risk checks
    bool checkPositionRisk(const std::string& symbol, double quantity, double price);
    bool checkPortfolioRisk(const std::vector<Position>& positions);

    // Risk metrics
    double calculateValueAtRisk(const std::vector<Position>& positions,
                               double confidenceLevel = 0.95);
    double calculateDrawdown(const std::vector<double>& equityCurve);
    double calculatePortfolioVolatility(const std::vector<Position>& positions);

    // Risk limits
    void setMaxDrawdown(double maxDrawdownPct);
    void setMaxPositionSize(double maxPositionSizePct);
    void setMaxConcentration(double maxConcentrationPct);
};

struct RiskConfig {
    double riskPerTrade;       // Risk per trade as percentage of equity
    double maxDrawdown;        // Maximum allowed drawdown percentage
    double maxPositionSize;    // Maximum position size as percentage of equity
    double maxConcentration;   // Maximum concentration in a single asset
    double maxCorrelation;     // Maximum correlation between portfolio assets
    double confidenceLevel;    // Confidence level for Value at Risk (VaR)
    int varTimeHorizon;        // Time horizon for VaR calculation in days
};

} // namespace risk
```

## Event System

The event system for handling market events.

```cpp
namespace event {

enum class EventType {
    BAR,
    TRADE,
    QUOTE,
    ORDER_BOOK,
    ORDER_UPDATE,
    POSITION_UPDATE,
    MARKET_STATE,
    TIMER,
    CUSTOM
};

class Event {
public:
    virtual EventType getType() const = 0;
    virtual datetime_t getTimestamp() const = 0;
    virtual ~Event() = default;
};

// Example of a specific event
class BarEvent : public Event {
public:
    BarEvent(const Bar& bar) : m_bar(bar) {}

    EventType getType() const override { return EventType::BAR; }
    datetime_t getTimestamp() const override { return m_bar.timestamp; }
    const Bar& getBar() const { return m_bar; }

private:
    Bar m_bar;
};

class EventDispatcher {
public:
    using EventHandler = std::function<void(const Event&)>;

    // Register handlers for event types
    void registerHandler(EventType type, EventHandler handler);

    // Dispatch an event to registered handlers
    void dispatch(const Event& event);

private:
    std::unordered_map<EventType, std::vector<EventHandler>> m_handlers;
};

} // namespace event
```

## Working with the API

### Creating a Custom Strategy

1. Inherit from the `Strategy` base class
2. Override the necessary event handlers
3. Implement your trading logic

### Strategy Lifecycle

1. `initialize`: Called once when the strategy is loaded
2. `onStart`: Called when the backtesting or live trading session starts
3. Event handlers: Called when corresponding market events occur
4. `onStop`: Called when the backtesting or live trading session ends

### Data Access Patterns

```cpp
// Get the latest N bars of data
auto data = getMarketData("BTCUSDT", DataFrequency::MINUTE, 100);

// Access specific data points
double lastClose = data.closes.back();
double previousClose = data.closes[data.closes.size() - 2];

// Calculate indicators
auto sma20 = indicators::SMA(data.closes, 20);
auto rsi14 = indicators::RSI(data.closes, 14);

// Access multiple timeframes
auto hourlyData = getMarketData("BTCUSDT", DataFrequency::HOUR, 24);
auto dailyData = getMarketData("BTCUSDT", DataFrequency::DAY, 30);
```

### Order Management Patterns

```cpp
// Market order
createOrder("BTCUSDT", OrderType::MARKET, OrderSide::BUY, 1.0);

// Limit order
createOrder("BTCUSDT", OrderType::LIMIT, OrderSide::BUY, 1.0, 50000.0);

// Stop order
auto order = createOrder("BTCUSDT", OrderType::STOP, OrderSide::SELL, 1.0, 0.0);
order.stopPrice = 45000.0;

// Cancel an order
cancelOrder(orderId);

// Modify an order
modifyOrder(orderId, 2.0, 51000.0);
```

### Position Management Patterns

```cpp
// Get current position
auto position = getPosition("BTCUSDT");

// Check position status
if (position.isLong()) {
    // Handle long position
} else if (position.isShort()) {
    // Handle short position
} else {
    // No position
}

// Close a position
if (!position.isFlat()) {
    createOrder("BTCUSDT", OrderType::MARKET,
               position.isLong() ? OrderSide::SELL : OrderSide::BUY,
               std::abs(position.quantity));
}
```

### Risk Management Patterns

```cpp
// Create a risk manager with config
RiskConfig riskConfig;
riskConfig.riskPerTrade = 0.01; // 1% risk per trade
riskConfig.maxDrawdown = 0.2;   // 20% max drawdown
RiskManager riskManager(riskConfig);

// Calculate position size based on stop loss
double price = data.closes.back();
double stopPrice = price * 0.95; // 5% stop loss
double positionSize = riskManager.calculatePositionSize("BTCUSDT", price, stopPrice);

// Check if a new position would exceed risk limits
if (riskManager.checkPositionRisk("BTCUSDT", positionSize, price)) {
    createOrder("BTCUSDT", OrderType::MARKET, OrderSide::BUY, positionSize);
}
```

### Performance Analysis Patterns

```cpp
// Get current performance metrics
auto metrics = getPerformance();

// Log key metrics
log(LogLevel::INFO, "Current equity: "
```
