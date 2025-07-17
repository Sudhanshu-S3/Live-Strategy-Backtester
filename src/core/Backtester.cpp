#include "../../include/core/Backtester.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <iomanip> // For std::setprecision

// --- MODIFICATION START: Include all necessary data handlers ---
#include "../../include/data/HFTDataHandler.h"
#include "../../include/data/WebSocketDataHandler.h"
#include "../../include/data/HistoricCSVDataHandler.h" // Assuming this also exists for other modes
// --- MODIFICATION END ---

#include "../../include/strategy/OrderBookImbalanceStrategy.h"
#include "../../include/strategy/MLStrategyClassifier.h"
#include "../../include/analytics/PerformanceForecaster.h"
#include "strategy/PairsTradingStrategy.h"
#include "../../include/execution/SimulatedExecutionHandler.h"
#include "../../include/strategy/StrategyFactory.h" // Add this near the top with other includes

// Safe JSON value extraction helper function to add
template<typename T>
T safe_get_value(const nlohmann::json& j, const std::string& key, const T& default_value) {
    if (j.contains(key)) {
        try {
            return j[key].get<T>();
        } catch (...) {
            return default_value;
        }
    }
    return default_value;
}

// ... (the create_strategy_from_config function remains unchanged) ...
std::shared_ptr<Strategy> create_strategy_from_config(
    const nlohmann::json& config,
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
    std::shared_ptr<DataHandler> data_handler
) {
    std::string name = config.contains("name") ? config["name"].get<std::string>() : "";
    
    // Safely access params
    nlohmann::json params = nlohmann::json::object();
    if (config.contains("params") && config["params"].is_object()) {
        params = config["params"];
    }

    if (name == "ORDER_BOOK_IMBALANCE") {
        // Safely get parameters with type checking
        int lookback_levels = 10;
        if (params.contains("lookback_levels") && params["lookback_levels"].is_number()) {
            lookback_levels = params["lookback_levels"].get<int>();
        }
        
        double imbalance_threshold = 1.5;
        if (params.contains("imbalance_threshold") && params["imbalance_threshold"].is_number()) {
            imbalance_threshold = params["imbalance_threshold"].get<double>();
        }
        
        return std::make_shared<OrderBookImbalanceStrategy>(
            event_queue, data_handler, config.value("symbol", ""),
            lookback_levels, imbalance_threshold
        );
    } else if (name == "PAIRS_TRADING") {
        auto symbols = config.value("symbols", std::vector<std::string>());
        if (symbols.size() < 2) {
            throw std::runtime_error("PairsTradingStrategy requires at least 2 symbols.");
        }
        
        // CORRECT WAY: Apply the same fix here for safety.
        int window = params.value("window", 50);
        double z_score_threshold = params.value("z_score_threshold", 2.0);

        return std::make_shared<PairsTradingStrategy>(
            event_queue, 
            data_handler, 
            name,
            symbols[0],
            symbols[1],
            window,
            z_score_threshold
        );
    }
    // Add other strategies here...
    
    throw std::runtime_error("Unknown or unsupported strategy type: " + name);
}


Backtester::Backtester(const nlohmann::json& config) : config_(config) {
    // Ensure required configuration sections exist to prevent null value errors
    if (!config_.contains("symbols") || !config_["symbols"].is_array() || config_["symbols"].empty()) {
        throw std::runtime_error("Config error: 'symbols' must be a non-empty array");
    }
    
    if (!config_.contains("data") || !config_["data"].is_object()) {
        config_["data"] = {
            {"start_date", "2025-07-13"},
            {"end_date", "2025-07-14"},
            {"trade_data_dir", "data"},
            {"book_data_dir", "data"},
            {"historical_data_fallback_dir", "historical_data"}
        };
    }
    
    if (!config_.contains("data_handler") || !config_["data_handler"].is_object()) {
        config_["data_handler"] = {
            {"live_host", "stream.binance.com"},
            {"live_port", "9443"},
            {"live_target", "/ws/btcusdt@trade"}
        };
    }
    
    if (!config_.contains("risk") || !config_["risk"].is_object()) {
        config_["risk"] = {
            {"risk_per_trade_pct", 0.01}
        };
    }
    
    if (!config_.contains("analytics") || !config_["analytics"].is_object()) {
        config_["analytics"] = {
            {"report_dir", "reports"}
        };
    }
    
    // Continue with the original constructor...
    std::string mode_str = config_.value("run_mode", "BACKTEST");
    if (mode_str == "OPTIMIZATION") run_mode_ = RunMode::OPTIMIZATION;
    else if (mode_str == "WALK_FORWARD") run_mode_ = RunMode::WALK_FORWARD;
    else if (mode_str == "SHADOW") run_mode_ = RunMode::SHADOW;
    else run_mode_ = RunMode::BACKTEST;

    event_queue_ = std::make_shared<ThreadSafeQueue<std::shared_ptr<Event>>>();
    auto symbols = config_["symbols"].get<std::vector<std::string>>();

    // --- MODIFICATION START: Conditional Data Handler Creation ---
    // This block replaces the original hardcoded HFTDataHandler creation.
    if (run_mode_ == RunMode::SHADOW) {
        std::cout << "Initializing WebSocketDataHandler for live session." << std::endl;
        
        // Create default data_handler if missing
        if (!config_.contains("data_handler") || config_["data_handler"].is_null()) {
            config_["data_handler"] = nlohmann::json::object();
        }
        
        const auto& dh_config = config_["data_handler"];
        
        // Get host with safe default
        std::string host = "stream.binance.com";  // Default value
        if (dh_config.contains("live_host") && !dh_config["live_host"].is_null()) {
            host = dh_config["live_host"].get<std::string>();
        } else {
            std::cout << "Using default WebSocket host: " << host << std::endl;
        }
        
        // Get port with safe default
        std::string port = "9443";  // Default value
        if (dh_config.contains("live_port") && !dh_config["live_port"].is_null()) {
            if (dh_config["live_port"].is_number()) {
                port = std::to_string(dh_config["live_port"].get<int>());
            } else if (dh_config["live_port"].is_string()) {
                port = dh_config["live_port"].get<std::string>();
            }
        } else {
            std::cout << "Using default WebSocket port: " << port << std::endl;
        }
        
        // Get target with safe default
        std::string target = "/ws";  // Default value
        if (dh_config.contains("live_target") && !dh_config["live_target"].is_null()) {
            target = dh_config["live_target"].get<std::string>();
        } else {
            std::cout << "Using default WebSocket target: " << target << std::endl;
        }
        
        // Create and connect the WebSocketDataHandler with guaranteed valid values
        data_handler_ = std::make_shared<WebSocketDataHandler>(
            event_queue_,
            symbols,
            host,
            port,
            target
        );
        
        // Save the updated config back (has the default values now)
        config_["data_handler"]["live_host"] = host;
        config_["data_handler"]["live_port"] = port;
        config_["data_handler"]["live_target"] = target;
        
        // Connect to the WebSocket
        std::static_pointer_cast<WebSocketDataHandler>(data_handler_)->connect();
        
        analytics_ = std::make_shared<Analytics>(config_["analytics"]);

        // Ensure strategies array exists
        if (!config_.contains("strategies") || !config_["strategies"].is_array()) {
            std::cout << "No strategies found in config, creating a default strategy for live trading" << std::endl;
            
            // Create a default strategy for the first symbol
            if (!config_["symbols"].empty()) {
                std::string first_symbol = config_["symbols"][0];
                
                // Create a default strategy configuration
                nlohmann::json default_strategy = {
                    {"name", "ORDER_BOOK_IMBALANCE"}, 
                    {"symbol", first_symbol},
                    {"active", true},
                    {"params", {
                        {"lookback_levels", 10},
                        {"imbalance_threshold", 1.5}
                    }}
                };
                
                // Create the strategies array and add default strategy
                config_["strategies"] = nlohmann::json::array();
                config_["strategies"].push_back(default_strategy);
            }
        }


        for (const auto& strategy_config : config_["strategies"]) {
            try {
                if (strategy_config.value("active", false)) {
                    strategies_.push_back(create_strategy_from_config(strategy_config, event_queue_, data_handler_));
                    analytics_->logDeployment(true);
                }
            } catch (const std::exception& e) {
                std::cerr << "Failed to deploy strategy: " << e.what() << std::endl;
                analytics_->logDeployment(false);
            }
        }

        portfolio_ = std::make_shared<Portfolio>(
            event_queue_, 
            config_.value("initial_capital", 100000.0), 
            data_handler_
        );
        
        execution_handler_ = std::make_shared<SimulatedExecutionHandler>(event_queue_, data_handler_);
        risk_manager_ = std::make_shared<RiskManager>(event_queue_, portfolio_, config_["risk"].value("risk_per_trade_pct", 0.01));
        
        // ... (The rest of the constructor remains the same) ...
        if (config_.contains("strategy_classifier")) {
            strategy_classifier_ = std::make_unique<MLStrategyClassifier>(
                config_["strategy_classifier"].value("model_path", "")
            );
        }

        if (config_.contains("performance_forecaster")) {
            performance_forecaster_ = std::make_unique<PerformanceForecaster>(
                config_["performance_forecaster"].value("model_path", "")
            );
        }

        if (config_.contains("market_regime")) {
            auto regime_config = config_["market_regime"];
            market_regime_detector_ = std::make_shared<MarketRegimeDetector>(
                event_queue_,
                data_handler_,
                regime_config.value("symbol", "BTC/USDT"),
                regime_config.value("volatility_lookback", 20),
                regime_config.value("trend_lookback", 50),
                regime_config.value("high_vol_threshold", 0.02),
                regime_config.value("low_vol_threshold", 0.005),
                regime_config.value("trend_threshold_pct", 0.5)
            );
        }

        monitor_interval_ms_ = config_.value("monitor_interval_ms", 5000);
        last_monitor_time_ = std::chrono::steady_clock::now();
        risk_check_interval_ms_ = config_.value("risk_check_interval_ms", 10000); // Default 10s
        last_risk_check_time_ = std::chrono::steady_clock::now();
        resource_check_interval_ms_ = config_.value("resource_check_interval_ms", 5000); // Default 5s
        last_resource_check_time_ = std::chrono::steady_clock::now();

        // --- MODIFICATION START: Initialize strategies from config ---
        if (config_.contains("strategies") && config_["strategies"].is_array()) {
            for (const auto& strategy_config : config_["strategies"]) {
                std::string strategy_name = strategy_config.value("name", "DEFAULT");
                std::cout << "Initializing strategy: " << strategy_name << std::endl;
                
                // Use StrategyFactory instead of direct Strategy instantiation
                auto strategy = StrategyFactory::createStrategy(
                    strategy_config,
                    event_queue_,
                    data_handler_
                );
                
                strategies_.push_back(strategy);
            }
        } else if (config_.contains("strategy")) {
            // Handle legacy single strategy config
            std::string strategy_name = config_["strategy"].value("name", "DEFAULT");
            std::cout << "Initializing strategy: " << strategy_name << std::endl;
            
            // Use StrategyFactory instead of direct Strategy instantiation
            auto strategy = StrategyFactory::createStrategy(
                config_["strategy"],
                event_queue_,
                data_handler_
            );
            
            strategies_.push_back(strategy);
        }
        // --- MODIFICATION END ---
    } else { // Default to historical data handling for BACKTEST, OPTIMIZATION, etc.
        std::cout << "Initializing HFTDataHandler for historical session." << std::endl;
        
        // Your original logic for historical data
        auto data_config = config_["data"]; 
        data_handler_ = std::make_shared<HFTDataHandler>(
            event_queue_, symbols,
            safe_get_value<std::string>(data_config, "trade_data_dir", ""),
            safe_get_value<std::string>(data_config, "book_data_dir", ""),
            safe_get_value<std::string>(data_config, "historical_data_fallback_dir", ""),
            safe_get_value<std::string>(data_config, "start_date", ""),
            safe_get_value<std::string>(data_config, "end_date", "")
        );
    }
    // --- MODIFICATION END ---
    
    analytics_ = std::make_shared<Analytics>(config_["analytics"]);

    // Ensure strategies array exists
    if (!config_.contains("strategies") || !config_["strategies"].is_array()) {
        std::cout << "No strategies found in config, creating a default strategy for live trading" << std::endl;
        
        // Create a default strategy for the first symbol
        if (!config_["symbols"].empty()) {
            std::string first_symbol = config_["symbols"][0];
            
            // Create a default strategy configuration
            nlohmann::json default_strategy = {
                {"name", "ORDER_BOOK_IMBALANCE"}, 
                {"symbol", first_symbol},
                {"active", true},
                {"params", {
                    {"lookback_levels", 10},
                    {"imbalance_threshold", 1.5}
                }}
            };
            
            // Create the strategies array and add default strategy
            config_["strategies"] = nlohmann::json::array();
            config_["strategies"].push_back(default_strategy);
        }
    }


    for (const auto& strategy_config : config_["strategies"]) {
        try {
            if (strategy_config.value("active", false)) {
                strategies_.push_back(create_strategy_from_config(strategy_config, event_queue_, data_handler_));
                analytics_->logDeployment(true);
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to deploy strategy: " << e.what() << std::endl;
            analytics_->logDeployment(false);
        }
    }

    portfolio_ = std::make_shared<Portfolio>(
        event_queue_, 
        config_.value("initial_capital", 100000.0), 
        data_handler_
    );
    
    execution_handler_ = std::make_shared<SimulatedExecutionHandler>(event_queue_, data_handler_);
    risk_manager_ = std::make_shared<RiskManager>(event_queue_, portfolio_, config_["risk"].value("risk_per_trade_pct", 0.01));
    
    // ... (The rest of the constructor remains the same) ...
    if (config_.contains("strategy_classifier")) {
        strategy_classifier_ = std::make_unique<MLStrategyClassifier>(
            config_["strategy_classifier"].value("model_path", "")
        );
    }

    if (config_.contains("performance_forecaster")) {
        performance_forecaster_ = std::make_unique<PerformanceForecaster>(
            config_["performance_forecaster"].value("model_path", "")
        );
    }

    if (config_.contains("market_regime")) {
        auto regime_config = config_["market_regime"];
        market_regime_detector_ = std::make_shared<MarketRegimeDetector>(
            event_queue_,
            data_handler_,
            regime_config.value("symbol", "BTC/USDT"),
            regime_config.value("volatility_lookback", 20),
            regime_config.value("trend_lookback", 50),
            regime_config.value("high_vol_threshold", 0.02),
            regime_config.value("low_vol_threshold", 0.005),
            regime_config.value("trend_threshold_pct", 0.5)
        );
    }

    monitor_interval_ms_ = config_.value("monitor_interval_ms", 5000);
    last_monitor_time_ = std::chrono::steady_clock::now();
    risk_check_interval_ms_ = config_.value("risk_check_interval_ms", 10000); // Default 10s
    last_risk_check_time_ = std::chrono::steady_clock::now();
    resource_check_interval_ms_ = config_.value("resource_check_interval_ms", 5000); // Default 5s
    last_resource_check_time_ = std::chrono::steady_clock::now();

    // --- MODIFICATION START: Initialize strategies from config ---
    if (config_.contains("strategies") && config_["strategies"].is_array()) {
        for (const auto& strategy_config : config_["strategies"]) {
            std::string strategy_name = strategy_config.value("name", "DEFAULT");
            std::cout << "Initializing strategy: " << strategy_name << std::endl;
            
            // Use StrategyFactory instead of direct Strategy instantiation
            auto strategy = StrategyFactory::createStrategy(
                strategy_config,
                event_queue_,
                data_handler_
            );
            
            strategies_.push_back(strategy);
        }
    } else if (config_.contains("strategy")) {
        // Handle legacy single strategy config
        std::string strategy_name = config_["strategy"].value("name", "DEFAULT");
        std::cout << "Initializing strategy: " << strategy_name << std::endl;
        
        // Use StrategyFactory instead of direct Strategy instantiation
        auto strategy = StrategyFactory::createStrategy(
            config_["strategy"],
            event_queue_,
            data_handler_
        );
        
        strategies_.push_back(strategy);
    }
    // --- MODIFICATION END ---
}

// ... (The rest of the Backtester.cpp file remains unchanged) ...
// The run(), run_backtest(), handleEvent(), and other methods are the same.
Backtester::~Backtester() {
    continue_backtest_ = false;
}

void Backtester::run() {
    switch (run_mode_) {
        case RunMode::OPTIMIZATION:
            run_optimization();
            break;
        case RunMode::WALK_FORWARD:
            run_walk_forward();
            break;
        case RunMode::BACKTEST:
        case RunMode::SHADOW:
        default:
            run_backtest();
            break;
    }

    if (run_mode_ == RunMode::SHADOW) {
        // For live trading, we need to process data as it comes in
        try {
            // Check if the data handler has already been initialized
            auto ws_handler = std::dynamic_pointer_cast<WebSocketDataHandler>(data_handler_);
            if (ws_handler) {
                // Set up a callback to process new data
                ws_handler->setOnNewDataCallback([this]() {
                    // Process any events in the queue
                    std::shared_ptr<Event> event;
                    // Replace event_queue_->empty() with a check using try_pop
                    while (auto event_opt = event_queue_->try_pop()) {
                        std::shared_ptr<Event> event = *event_opt.value();  // Dereference to get the inner shared_ptr
                        if (event) {
                            this->handleEvent(event);
                        }
                    }
                    
                    // Log current performance periodically
                    static auto last_log = std::chrono::steady_clock::now();
                    auto now = std::chrono::steady_clock::now();
                    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_log).count() >= 5) {
                        log_live_performance();
                        last_log = now;
                    }
                });
                
                // Keep the program running until externally stopped
                finished_ = false;
                while (!finished_) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error in live trading loop: " << e.what() << std::endl;
        }
    } else {
        // Original backtest logic remains unchanged
        run_backtest();
    }
}

void Backtester::run_backtest() {
    std::cout << "Backtester starting in " << (run_mode_ == RunMode::SHADOW ? "SHADOW" : "BACKTEST") << " mode..." << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();
    long long event_count = 0;

    // This old connection logic is no longer needed here, it's handled in the constructor.
    /*
    if (run_mode_ == RunMode::SHADOW) {
        std::dynamic_pointer_cast<HFTDataHandler>(data_handler_)->connectLiveFeed();
    }
    */
    
    while (continue_backtest_ && (!data_handler_->isFinished() || run_mode_ == RunMode::SHADOW)) {
        data_handler_->updateBars();
        
        analytics_->detect_anomalies(data_handler_);
        
        while (auto opt_event = event_queue_->try_pop()) {
            std::shared_ptr<Event> event = *opt_event.value();
            handleEvent(event);
            event_count++;
        }

        if (run_mode_ == RunMode::SHADOW) {
            log_live_performance();

            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_risk_check_time_).count() > risk_check_interval_ms_) {
                risk_manager_->monitorRealTimeRisk();
                last_risk_check_time_ = now;
            }
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_resource_check_time_).count() > resource_check_interval_ms_) {
                analytics_->snapshotSystemResources();
                last_resource_check_time_ = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    continue_backtest_ = false;
    std::cout << "Backtester event loop finished." << std::endl;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    portfolio_->generateReport();
    analytics_->generateReport(portfolio_);
    analytics_->generateMarketConditionReport(portfolio_);
    analytics_->generateFactorAnalysisReport(portfolio_);
    analytics_->generateDeploymentReport();
    analytics_->generateResourceUsageReport();

    if (performance_forecaster_) {
        auto forecast = performance_forecaster_->forecast_performance(*portfolio_, 10);
        std::cout << "\n--- Performance Forecast ---\n";
        std::cout << "Predicted Sharpe Ratio: " << forecast.predicted_sharpe << std::endl;
        std::cout << "Predicted Max Drawdown: " << forecast.predicted_max_drawdown << std::endl;
        std::cout << "Equity Forecast for next 10 periods:" << std::endl;
        for (size_t i = 0; i < forecast.equity_forecast.size(); ++i) {
            std::cout << "  Period " << i + 1 << ": " << forecast.equity_forecast[i] << std::endl;
        }
        std::cout << "---------------------------\n";
    }

    std::cout << "\n--- System Metrics ---\n";
    std::cout << "Backtest Execution Time: " << duration << " ms\n";
    if (duration > 0) {
        double throughput = static_cast<double>(event_count) / duration * 1000.0;
        std::cout << "Event Throughput: " << std::fixed << std::setprecision(2) << throughput << " events/sec\n";
    }
    std::cout << "----------------------\n";
}

void Backtester::handleEvent(const std::shared_ptr<Event>& event) {
    portfolio_->updateTimeIndex();
    
    if (event->type == EventType::MARKET) {
        for (auto& strategy : strategies_) {
            strategy->onMarket(static_cast<MarketEvent&>(*event));
        }
    } else if (event->type == EventType::TRADE) {
        for (auto& strategy : strategies_) {
            strategy->onTrade(static_cast<TradeEvent&>(*event));
        }
    } else if (event->type == EventType::ORDER_BOOK) {
        for (auto& strategy : strategies_) {
            strategy->onOrderBook(static_cast<OrderBookEvent&>(*event));
        }
    } else if (event->type == EventType::MARKET_REGIME_CHANGED) {
        auto& regime_event = static_cast<MarketRegimeChangedEvent&>(*event);
        portfolio_->onMarketRegimeChanged(regime_event);
        for (auto& strategy : strategies_) {
            strategy->onMarketRegimeChanged(regime_event);
        }

        if (strategy_classifier_) {
            auto recommended_strategies = strategy_classifier_->classify(regime_event.new_state);
            for (auto& strategy : strategies_) {
                bool recommended = std::find(recommended_strategies.begin(), recommended_strategies.end(), strategy->getName()) != recommended_strategies.end();
                if (recommended) {
                    strategy->resume();
                } else {
                    strategy->pause();
                }
            }
        }
    }

    switch (event->type) {
        case EventType::SIGNAL:
            risk_manager_->onSignal(static_cast<SignalEvent&>(*event));
            break;
        case EventType::ORDER:
            execution_handler_->onOrder(static_cast<OrderEvent&>(*event));
            break;
        case EventType::FILL:
            portfolio_->onFill(static_cast<FillEvent&>(*event));
            break;
        case EventType::DATA_SOURCE_STATUS:
            risk_manager_->onDataSourceStatus(static_cast<DataSourceStatusEvent&>(*event));
            break;
        default:
            break;
    }
}

void Backtester::log_live_performance() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_monitor_time_).count();

    if (elapsed > monitor_interval_ms_) {
        last_monitor_time_ = now;
        
        double pnl = portfolio_->getRealTimePnL();
        auto positions = portfolio_->getCurrentPositions();

        printf("\n--- LIVE STATUS UPDATE ---\n");
        printf("Timestamp: %lld\n", std::chrono::system_clock::now().time_since_epoch().count());
        printf("Real-Time P&L: %.2f\n", pnl);
        printf("Current Positions:\n");
        if (positions.empty()) {
            printf("  (No open positions)\n");
        } else {
            for (const auto& [symbol, pos] : positions) {
                printf("  - %s: Quantity=%.4f, AvgCost=%.2f, Dir=%s\n",
                       symbol.c_str(), pos.quantity, pos.average_cost, 
                       (static_cast<int>(pos.direction) == static_cast<int>(OrderSide::BUY) ? "BUY" : "SELL"));
            }
        }
        printf("--------------------------\n\n");
    }
}

nlohmann::json Backtester::run_optimization() {
    std::cout << "\n--- RUNNING PARAMETER OPTIMIZATION ---\n" << std::endl;
    
    if (!config_.contains("optimization")) {
        std::cerr << "Optimization requires an 'optimization' section in config.json" << std::endl;
        return nlohmann::json();
    }
    auto opt_config = config_["optimization"];
    std::string strategy_to_optimize = opt_config.value("strategy_to_optimize", "");
    if (strategy_to_optimize.empty()) {
        std::cerr << "Optimization config must specify 'strategy_to_optimize'" << std::endl;
        return nlohmann::json();
    }

    nlohmann::json base_strategy_config;
    int strategy_idx = -1;
    for (size_t i = 0; i < config_["strategies"].size(); ++i) {
        if (config_["strategies"][i]["name"] == strategy_to_optimize) {
            base_strategy_config = config_["strategies"][i];
            strategy_idx = i;
            break;
        }
    }
    if (strategy_idx == -1) {
        std::cerr << "Could not find strategy '" << strategy_to_optimize << "' in config." << std::endl;
        return nlohmann::json();
    }

    std::vector<nlohmann::json> parameter_sets;
    auto param_ranges = opt_config["param_ranges"];
    for (double p1 = param_ranges["p1_start"].get<double>(); p1 <= param_ranges["p1_end"].get<double>(); p1 += param_ranges["p1_step"].get<double>()) {
        for (double p2 = param_ranges["p2_start"].get<double>(); p2 <= param_ranges["p2_end"].get<double>(); p2 += param_ranges["p2_step"].get<double>()) {
            nlohmann::json params;
            params[param_ranges["p1_name"].get<std::string>()] = p1;
            params[param_ranges["p2_name"].get<std::string>()] = p2;
            parameter_sets.push_back(params);
        }
    }

    double best_performance = -1e9;
    nlohmann::json best_params;

    for (const auto& params : parameter_sets) {
        std::cout << "Testing params: " << params.dump() << std::endl;

        nlohmann::json run_config = config_;
        run_config["strategies"][strategy_idx]["params"] = params;
        
        Backtester backtester(run_config);
        backtester.run_backtest();
        
        double perf = backtester.getPortfolio()->getRealTimePerformance().getSharpeRatio();
        if (perf > best_performance) {
            best_performance = perf;
            best_params = params;
        }
    }

    std::cout << "\n--- Optimization Results ---\n";
    std::cout << "Best Sharpe Ratio: " << best_performance << std::endl;
    std::cout << "Best Parameters: " << best_params.dump(4) << std::endl;
    std::cout << "---------------------------\n";

    return best_params;
}

void Backtester::run_walk_forward() {
    std::cout << "\n--- RUNNING WALK-FORWARD ANALYSIS ---\n" << std::endl;
    
    if (!config_.contains("walk_forward")) {
        std::cerr << "Walk-forward analysis requires a 'walk_forward' section in config.json" << std::endl;
        return;
    }
    auto wf_config = config_["walk_forward"];
    int num_splits = wf_config.value("num_splits", 5);
    int in_sample_days = wf_config.value("in_sample_days", 90);
    int out_of_sample_days = wf_config.value("out_of_sample_days", 30);
    std::string start_date_str = wf_config.value("start_date", "2023-01-01");

    auto string_to_time = [](const std::string& s) {
        std::tm tm = {};
        std::stringstream ss(s);
        ss >> std::get_time(&tm, "%Y-%m-%d");
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    };
    auto time_to_string = [](const std::chrono::system_clock::time_point& tp) {
        std::time_t t = std::chrono::system_clock::to_time_t(tp);
        char buffer[11];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", std::localtime(&t));
        return std::string(buffer);
    };

    std::vector<double> out_of_sample_returns;
    auto current_start_date = string_to_time(start_date_str);

    for (int i = 0; i < num_splits; ++i) {
        std::cout << "\n--- WFA Split " << i + 1 << "/" << num_splits << " ---\n";

        auto in_sample_start = current_start_date;
        auto in_sample_end = in_sample_start + std::chrono::hours(24 * in_sample_days);
        
        auto out_of_sample_start = in_sample_end;
        auto out_of_sample_end = out_of_sample_start + std::chrono::hours(24 * out_of_sample_days);

        std::cout << "  In-Sample: " << time_to_string(in_sample_start) << " to " << time_to_string(in_sample_end) << std::endl;
        std::cout << "  Out-of-Sample: " << time_to_string(out_of_sample_start) << " to " << time_to_string(out_of_sample_end) << std::endl;
        
        nlohmann::json in_sample_config = config_;
        in_sample_config["run_mode"] = "OPTIMIZATION";
        in_sample_config["data"]["start_date"] = time_to_string(in_sample_start);
        in_sample_config["data"]["end_date"] = time_to_string(in_sample_end);
        
        Backtester in_sample_backtester(in_sample_config);
        nlohmann::json best_params = in_sample_backtester.run_optimization();

        if (best_params.empty()) {
            std::cerr << "Optimization failed for split " << i + 1 << ", skipping." << std::endl;
            current_start_date = out_of_sample_start;
            continue;
        }

        nlohmann::json out_of_sample_config = config_;
        out_of_sample_config["run_mode"] = "BACKTEST";
        out_of_sample_config["data"]["start_date"] = time_to_string(out_of_sample_start);
        out_of_sample_config["data"]["end_date"] = time_to_string(out_of_sample_end);
        
        std::string strategy_to_optimize = config_["optimization"].value("strategy_to_optimize", "");
        for (size_t j = 0; j < out_of_sample_config["strategies"].size(); ++j) {
            if (out_of_sample_config["strategies"][j]["name"] == strategy_to_optimize) {
                out_of_sample_config["strategies"][j]["params"] = best_params;
                break;
            }
        }
        
        Backtester out_of_sample_backtester(out_of_sample_config);
        out_of_sample_backtester.run_backtest();
        
        double total_return = out_of_sample_backtester.getPortfolio()->getRealTimePerformance().getTotalReturn();
        out_of_sample_returns.push_back(total_return);
        
        current_start_date = out_of_sample_start;
    }

    std::cout << "\n--- Walk-Forward Analysis Results ---\n";
    double total_return_sum = 0;
    for(size_t i = 0; i < out_of_sample_returns.size(); ++i) {
        printf("Split %zu Return: %.2f%%\n", i + 1, out_of_sample_returns[i] * 100);
        total_return_sum += out_of_sample_returns[i];
    }
    double avg_return = out_of_sample_returns.empty() ? 0 : total_return_sum / out_of_sample_returns.size();
    printf("\nAverage Out-of-Sample Return: %.2f%%\n", avg_return * 100);
    std::cout << "-------------------------------------\n";
}