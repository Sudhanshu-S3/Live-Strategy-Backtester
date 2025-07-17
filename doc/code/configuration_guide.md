# Configuration Guide

## Overview

This guide documents the configuration system for the Live Strategy Backtester. Proper configuration is essential for accurate backtesting and effective strategy execution.

## Configuration File Format

The Live Strategy Backtester uses JSON as its primary configuration format. Configuration files are structured hierarchically with sections for different system components.

### Basic Configuration Structure

```json
{
  "general": {
    "name": "My Backtest",
    "description": "Testing Moving Average Crossover Strategy",
    "start_date": "2025-01-01",
    "end_date": "2025-07-01"
  },
  "data": {
    // Data source configuration
  },
  "strategy": {
    // Strategy configuration
  },
  "execution": {
    // Execution model configuration
  },
  "risk": {
    // Risk management configuration
  },
  "reporting": {
    // Reporting configuration
  }
}
```

## Configuration Sections

### General Configuration

```json
"general": {
  "name": "MA Crossover BTC",
  "description": "Moving average crossover strategy for Bitcoin",
  "start_date": "2025-01-01T00:00:00",
  "end_date": "2025-07-01T00:00:00",
  "timezone": "UTC",
  "initial_capital": 100000.0,
  "base_currency": "USD"
}
```

| Parameter         | Type   | Description                    | Default            |
| ----------------- | ------ | ------------------------------ | ------------------ |
| `name`            | string | Name of the backtest           | "Unnamed Backtest" |
| `description`     | string | Description of the backtest    | ""                 |
| `start_date`      | string | Start date in ISO format       | Required           |
| `end_date`        | string | End date in ISO format         | Required           |
| `timezone`        | string | Timezone for date calculations | "UTC"              |
| `initial_capital` | number | Initial capital amount         | 100000.0           |
| `base_currency`   | string | Base currency for calculations | "USD"              |

### Data Configuration

#### CSV Data Source

```json
"data": {
  "source": "csv",
  "files": [
    {
      "path": "data/BTCUSDT-1s-2025-07-13.csv",
      "instrument": "BTCUSDT",
      "format": "time,open,high,low,close,volume",
      "time_format": "%Y-%m-%dT%H:%M:%S",
      "delimiter": ","
    }
  ],
  "cache": {
    "enabled": true,
    "max_size_mb": 1024
  }
}
```

#### Database Data Source

```json
"data": {
  "source": "database",
  "connection": {
    "type": "postgresql",
    "host": "localhost",
    "port": 5432,
    "database": "market_data",
    "username": "user",
    "password": "password"
  },
  "query": "SELECT timestamp, open, high, low, close, volume FROM ohlcv WHERE instrument = 'BTCUSDT' AND timestamp BETWEEN ? AND ?"
}
```

#### Live Data Source

```json
"data": {
  "source": "live",
  "provider": "binance",
  "api_key": "your_api_key",
  "api_secret": "your_api_secret",
  "instruments": ["BTCUSDT", "ETHUSDT"],
  "interval": "1m"
}
```

### Strategy Configuration

```json
"strategy": {
  "name": "MovingAverageCrossover",
  "parameters": {
    "short_period": 20,
    "long_period": 50,
    "instruments": ["BTCUSDT"],
    "position_sizing": "fixed_amount",
    "position_size": 10000.0,
    "stop_loss_pct": 0.02,
    "take_profit_pct": 0.05
  }
}
```

Common strategy parameters:

| Parameter         | Type   | Description                   | Default         |
| ----------------- | ------ | ----------------------------- | --------------- |
| `name`            | string | Strategy class name           | Required        |
| `instruments`     | array  | Array of instruments to trade | Required        |
| `position_sizing` | string | Position sizing method        | "fixed_amount"  |
| `position_size`   | number | Position size value           | Required        |
| `stop_loss_pct`   | number | Stop loss percentage          | null (disabled) |
| `take_profit_pct` | number | Take profit percentage        | null (disabled) |

### Execution Configuration

```json
"execution": {
  "model": "market_impact",
  "slippage": {
    "type": "fixed",
    "value": 0.001
  },
  "commission": {
    "type": "percentage",
    "value": 0.001
  },
  "delay": {
    "type": "fixed",
    "value_ms": 100
  },
  "volume_restrictions": {
    "max_volume_percent": 0.1
  }
}
```

Execution model types:

- `perfect`: No slippage or market impact
- `fixed_slippage`: Fixed slippage percentage
- `market_impact`: Models market impact based on order size
- `order_book`: Simulates order book depth and liquidity

Commission types:

- `fixed`: Fixed fee per trade
- `percentage`: Percentage of trade value
- `tiered`: Tiered fee structure based on volume

### Risk Management Configuration

```json
"risk": {
  "max_position_size_percent": 0.1,
  "max_position_size_absolute": 100000.0,
  "max_drawdown_percent": 0.2,
  "max_leverage": 3.0,
  "correlation_matrix": {
    "path": "config/correlation_matrix.csv"
  },
  "volatility_scaling": {
    "enabled": true,
    "target_volatility": 0.15,
    "lookback_days": 20
  }
}
```

### Reporting Configuration

```json
"reporting": {
  "output_path": "results",
  "formats": ["csv", "json", "html"],
  "metrics": [
    "total_return",
    "sharpe_ratio",
    "max_drawdown",
    "win_rate",
    "profit_factor"
  ],
  "plots": [
    "equity_curve",
    "drawdowns",
    "monthly_returns"
  ],
  "trade_log": true,
  "detailed_statistics": true
}
```

## Configuration Best Practices

1. **Isolate environment-specific settings**: Use separate config files for development, testing, and production
2. **Version control your configs**: Store base configuration files in version control
3. **Secure sensitive information**: Keep API keys and credentials in environment variables or secure storage
4. **Document custom parameters**: Add comments for custom or non-standard parameters
5. **Validate configurations**: Use the configuration validation tools to check for errors

## Overriding Configuration

Configuration values can be overridden using:

### Command Line Arguments

```bash
./backtester --config config.json --param general.initial_capital=200000 --param strategy.parameters.short_period=15
```

### Environment Variables

```bash
export BT_INITIAL_CAPITAL=200000
export BT_STRATEGY_SHORT_PERIOD=15
./backtester --config config.json
```

### Configuration Precedence

Configuration values are applied in the following order (highest priority last):

1. Default values
2. Configuration file
3. Environment variables
4. Command line arguments

## Configuration Templates

### Simple Moving Average Crossover

```json
{
  "general": {
    "name": "SMA Crossover",
    "initial_capital": 100000.0
  },
  "data": {
    "source": "csv",
    "files": [
      {
        "path": "data/BTCUSDT-1h.csv",
        "instrument": "BTCUSDT"
      }
    ]
  },
  "strategy": {
    "name": "MovingAverageCrossover",
    "parameters": {
      "short_period": 20,
      "long_period": 50,
      "instruments": ["BTCUSDT"],
      "position_size": 10000.0
    }
  }
}
```

### Multi-Asset Portfolio Strategy

```json
{
  "general": {
    "name": "Multi-Asset Portfolio",
    "initial_capital": 1000000.0
  },
  "data": {
    "source": "csv",
    "files": [
      { "path": "data/BTCUSDT-1d.csv", "instrument": "BTCUSDT" },
      { "path": "data/ETHUSDT-1d.csv", "instrument": "ETHUSDT" },
      { "path": "data/SOLUSDT-1d.csv", "instrument": "SOLUSDT" }
    ]
  },
  "strategy": {
    "name": "MomentumPortfolio",
    "parameters": {
      "lookback_period": 90,
      "rebalance_days": 30,
      "instruments": ["BTCUSDT", "ETHUSDT", "SOLUSDT"],
      "position_sizing": "equal_weight"
    }
  },
  "risk": {
    "volatility_scaling": {
      "enabled": true,
      "target_volatility": 0.15
    }
  }
}
```

## Configuration Validation

The system includes a configuration validation tool to check for errors and inconsistencies:

```bash
# Validate a configuration file
./backtester --validate-config config.json

# Output detailed validation information
./backtester --validate-config config.json --verbose
```

Common validation errors:

- Missing required fields
- Type mismatches (string vs number)
- Invalid date formats
- Invalid parameter combinations
- Missing referenced files
