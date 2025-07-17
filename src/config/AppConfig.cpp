#include "../../include/config/AppConfig.h"
#include <iomanip>
#include <stdexcept>
#include <iostream>

AppConfig AppConfig::loadFromFile(const std::string& filename) {
    AppConfig config;
    
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open config file: " + filename);
        }
        
        nlohmann::json j;
        file >> j;
        
        // Parse run mode safely
        if (j.contains("run_mode") && j["run_mode"].is_string()) {
            config.run_mode = stringToRunMode(j["run_mode"].get<std::string>());
        }
        
        // Parse symbols safely
        if (j.contains("symbols")) {
            if (j["symbols"].is_array()) {
                for (const auto& symbol : j["symbols"]) {
                    if (symbol.is_string()) {
                        config.symbols.push_back(symbol.get<std::string>());
                    }
                }
            } else if (j["symbols"].is_string()) {
                // Handle case where symbols might be a single string
                config.symbols.push_back(j["symbols"].get<std::string>());
            }
        }
        
        // Parse initial capital safely
        if (j.contains("initial_capital") && j["initial_capital"].is_number()) {
            config.initial_capital = j["initial_capital"].get<double>();
        }
        
        // Parse data config safely
        if (j.contains("data") && j["data"].is_object()) {
            auto& data = j["data"];
            if (data.contains("start_date") && data["start_date"].is_string())
                config.data.start_date = data["start_date"].get<std::string>();
            
            if (data.contains("end_date") && data["end_date"].is_string())
                config.data.end_date = data["end_date"].get<std::string>();
            
            if (data.contains("trade_data_dir") && data["trade_data_dir"].is_string())
                config.data.trade_data_dir = data["trade_data_dir"].get<std::string>();
            
            if (data.contains("book_data_dir") && data["book_data_dir"].is_string())
                config.data.book_data_dir = data["book_data_dir"].get<std::string>();
            
            if (data.contains("historical_data_fallback_dir") && data["historical_data_fallback_dir"].is_string())
                config.data.historical_data_fallback_dir = data["historical_data_fallback_dir"].get<std::string>();
        }
        
        // Parse strategies safely
        if (j.contains("strategies") && j["strategies"].is_array()) {
            for (const auto& strategy_json : j["strategies"]) {
                if (strategy_json.is_object()) {
                    StrategyConfig strategy;
                    
                    if (strategy_json.contains("name") && strategy_json["name"].is_string())
                        strategy.name = strategy_json["name"].get<std::string>();
                    
                    if (strategy_json.contains("symbol") && strategy_json["symbol"].is_string())
                        strategy.symbol = strategy_json["symbol"].get<std::string>();
                    
                    if (strategy_json.contains("active") && strategy_json["active"].is_boolean())
                        strategy.active = strategy_json["active"].get<bool>();
                    
                    if (strategy_json.contains("params") && strategy_json["params"].is_object()) {
                        auto& params = strategy_json["params"];
                        
                        if (params.contains("lookback_levels") && params["lookback_levels"].is_number_integer())
                            strategy.params.lookback_levels = params["lookback_levels"].get<int>();
                        
                        if (params.contains("imbalance_threshold") && params["imbalance_threshold"].is_number())
                            strategy.params.imbalance_threshold = params["imbalance_threshold"].get<double>();
                    }
                    
                    config.strategies.push_back(strategy);
                }
            }
        }
        
        // Parse WebSocket config safely
        if (j.contains("websocket") && j["websocket"].is_object()) {
            auto& ws = j["websocket"];
            
            if (ws.contains("host") && ws["host"].is_string())
                config.websocket.host = ws["host"].get<std::string>();
            
            if (ws.contains("port") && ws["port"].is_number_integer())
                config.websocket.port = ws["port"].get<int>();
            
            if (ws.contains("target") && ws["target"].is_string())
                config.websocket.target = ws["target"].get<std::string>();
        }
        
        // Parse risk config safely
        if (j.contains("risk") && j["risk"].is_object()) {
            auto& risk = j["risk"];
            
            if (risk.contains("risk_per_trade_pct") && risk["risk_per_trade_pct"].is_number())
                config.risk.risk_per_trade_pct = risk["risk_per_trade_pct"].get<double>();
            
            if (risk.contains("max_drawdown_pct") && risk["max_drawdown_pct"].is_number())
                config.risk.max_drawdown_pct = risk["max_drawdown_pct"].get<double>();
        }
        
        // If there are no strategies but strategy name is defined at top level, create one
        if (config.strategies.empty() && j.contains("strategy") && j["strategy"].is_string()) {
            StrategyConfig strategy;
            strategy.name = j["strategy"].get<std::string>();
            
            // Use the first symbol if available
            if (!config.symbols.empty()) {
                strategy.symbol = config.symbols[0];
            }
            
            config.strategies.push_back(strategy);
        }
        
        return config;
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        // Return default config instead of throwing
        return config;
    } catch (const std::exception& e) {
        std::cerr << "Error loading configuration: " << e.what() << std::endl;
        // Return default config instead of throwing
        return config;
    }
}

void AppConfig::saveToFile(const std::string& filename) const {
    nlohmann::json j;
    
    // Convert run mode to string
    j["run_mode"] = runModeToString(run_mode);
    
    // Save symbols
    j["symbols"] = symbols;
    j["initial_capital"] = initial_capital;
    
    // Save data config
    j["data"] = {
        {"start_date", data.start_date},
        {"end_date", data.end_date},
        {"trade_data_dir", data.trade_data_dir},
        {"book_data_dir", data.book_data_dir},
        {"historical_data_fallback_dir", data.historical_data_fallback_dir}
    };
    
    // Save strategies
    j["strategies"] = nlohmann::json::array();
    for (const auto& strategy : strategies) {
        nlohmann::json strategy_json = {
            {"name", strategy.name},
            {"symbol", strategy.symbol},
            {"active", strategy.active},
            {"params", {
                {"lookback_levels", strategy.params.lookback_levels},
                {"imbalance_threshold", strategy.params.imbalance_threshold}
            }}
        };
        j["strategies"].push_back(strategy_json);
    }
    
    // Save WebSocket config
    j["websocket"] = {
        {"host", websocket.host},
        {"port", websocket.port},
        {"target", websocket.target}
    };
    
    // Save risk config
    j["risk"] = {
        {"risk_per_trade_pct", risk.risk_per_trade_pct},
        {"max_drawdown_pct", risk.max_drawdown_pct}
    };
    
    // Write to file
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file for writing: " + filename);
        }
        file << std::setw(4) << j << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error saving configuration: " << e.what() << std::endl;
        throw;
    }
}