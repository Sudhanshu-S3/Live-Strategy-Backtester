#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

enum class RunMode {
    BACKTEST,
    LIVE_TRADING,
    SHADOW,             // Add SHADOW for compatibility with existing code
    OPTIMIZATION,
    WALK_FORWARD,
    MONTE_CARLO
};

struct DataConfig {
    std::string start_date;
    std::string end_date;
    std::string trade_data_dir = "data";
    std::string book_data_dir = "data";
    std::string historical_data_fallback_dir = "historical_data";
};

struct StrategyParams {
    int lookback_levels = 10;
    double imbalance_threshold = 1.5;
    // Add other strategy parameters as needed
};

struct StrategyConfig {
    std::string name;
    std::string symbol;
    bool active = true;
    StrategyParams params;
};

struct WebSocketConfig {
    std::string host = "stream.binance.com";
    int port = 9443;
    std::string target = "/ws";
};

struct RiskConfig {
    double risk_per_trade_pct = 0.01;
    double max_drawdown_pct = 0.05;
    // Add other risk parameters
};

struct AppConfig {
    RunMode run_mode = RunMode::BACKTEST;
    std::vector<std::string> symbols;
    double initial_capital = 100000.0;
    DataConfig data;
    std::vector<StrategyConfig> strategies;
    RiskConfig risk;
    WebSocketConfig websocket;
    
    // Convert RunMode to string for display
    static std::string runModeToString(RunMode mode) {
        switch (mode) {
            case RunMode::BACKTEST: return "BACKTEST";
            case RunMode::LIVE_TRADING: return "LIVE_TRADING";
            case RunMode::SHADOW: return "SHADOW";  // Add SHADOW case
            case RunMode::OPTIMIZATION: return "OPTIMIZATION";
            case RunMode::WALK_FORWARD: return "WALK_FORWARD";
            case RunMode::MONTE_CARLO: return "MONTE_CARLO";
            default: return "UNKNOWN";
        }
    }
    
    // Convert string to RunMode for parsing
    static RunMode stringToRunMode(const std::string& mode) {
        if (mode == "BACKTEST") return RunMode::BACKTEST;
        if (mode == "LIVE_TRADING") return RunMode::LIVE_TRADING;
        if (mode == "SHADOW") return RunMode::SHADOW;  // Add SHADOW case
        if (mode == "OPTIMIZATION") return RunMode::OPTIMIZATION;
        if (mode == "WALK_FORWARD") return RunMode::WALK_FORWARD;
        if (mode == "MONTE_CARLO") return RunMode::MONTE_CARLO;
        return RunMode::BACKTEST; // Default
    }
    
    // Load from file - declaration only
    static AppConfig loadFromFile(const std::string& filename);
    
    // Save to file - declaration only
    void saveToFile(const std::string& filename) const;
};

#endif // APP_CONFIG_H