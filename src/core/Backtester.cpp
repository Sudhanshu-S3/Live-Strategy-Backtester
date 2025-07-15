#include "../../include/core/Backtester.h"
#include "../../include/data/HFTDataHandler.h"
#include "../../include/strategy/OrderBookImbalanceStrategy.h"
#include "../../include/strategy/MLStrategyClassifier.h"
#include "../../include/analytics/PerformanceForecaster.h"
#include "strategy/PairsTradingStrategy.h" // Add include
#include <iostream>
#include <fstream>
#include <stdexcept>

std::shared_ptr<Strategy> create_strategy_from_config(
    const nlohmann::json& config,
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
    std::shared_ptr<DataHandler> data_handler
) {
    std::string name = config.value("name", "");
    if (name == "ORDER_BOOK_IMBALANCE") {
        auto params = config["params"];
        return std::make_shared<OrderBookImbalanceStrategy>(
            event_queue, data_handler, config.value("symbol", ""),
            params.value("lookback_levels", 10), params.value("imbalance_threshold", 1.5)
        );
    } else if (name == "PAIRS_TRADING") {
        auto params = config["params"];
        return std::make_shared<PairsTradingStrategy>(
            event_queue, data_handler, config["symbols"].get<std::vector<std::string>>(),
            params.value("lookback", 50), 
            params.value("entry_z", 2.0),
            params.value("exit_z", 0.5)
        );
    } else if (name == "MARKET_REGIME_DETECTOR") {
        auto params = config["params"];
        return std::make_shared<MarketRegimeDetector>(
            event_queue, data_handler, config.value("symbol", ""),
            params.value("volatility_lookback", 20),
            params.value("trend_lookback", 50),
            params.value("high_vol_threshold", 0.02),
            params.value("low_vol_threshold", 0.005),
            params.value("trend_threshold_pct", 0.5)
        );
    }
    throw std::runtime_error("Unknown strategy name in config: " + name);
}

Backtester::Backtester(const nlohmann::json& config) : config_(config) {
    std::string mode_str = config.value("run_mode", "BACKTEST");
    if (mode_str == "OPTIMIZATION") run_mode_ = RunMode::OPTIMIZATION;
    else if (mode_str == "WALK_FORWARD") run_mode_ = RunMode::WALK_FORWARD;
    else if (mode_str == "SHADOW") run_mode_ = RunMode::SHADOW;
    else run_mode_ = RunMode::BACKTEST;

    event_queue_ = std::make_shared<ThreadSafeQueue<std::shared_ptr<Event>>>();
    
    auto data_config = config_["data"];
    data_handler_ = std::make_shared<HFTDataHandler>(
        event_queue_, config_["symbols"].get<std::vector<std::string>>(),
        data_config.value("trade_data_dir", ""),
        data_config.value("book_data_dir", ""),
        data_config.value("historical_data_fallback_dir", ""),
        data_config.value("start_date", ""),
        data_config.value("end_date", "")
    );

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
    analytics_ = std::make_shared<Analytics>(config_["analytics"]);

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
}

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
}

void Backtester::run_backtest() {
    std::cout << "Backtester starting in " << (run_mode_ == RunMode::SHADOW ? "SHADOW" : "BACKTEST") << " mode..." << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();
    long long event_count = 0;

    if (run_mode_ == RunMode::SHADOW) {
        // This cast is problematic if we want to support multiple live handlers.
        // Consider a more generic connect method in the DataHandler interface.
        // std::dynamic_pointer_cast<HFTDataHandler>(data_handler_)->connectLiveFeed();
    }
    
    while (continue_backtest_ && (!data_handler_->isFinished() || run_mode_ == RunMode::SHADOW)) {
        data_handler_->updateBars();
        
        analytics_->detect_anomalies(data_handler_);

        std::shared_ptr<Event> event;
        while (event_queue_->try_pop(event)) {
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
    
    // Pass market data events to all strategies
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
                bool recommended = false;
                for (const auto& recommended_name : recommended_strategies) {
                    if (strategy->getName() == recommended_name) {
                        recommended = true;
                        break;
                    }
                }
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
        case Event::ORDER:
            execution_handler_->onOrder(static_cast<OrderEvent&>(*event));
            break;
        case Event::FILL:
            portfolio_->onFill(static_cast<FillEvent&>(*event));
            break;
        case EventType::DATA_SOURCE_STATUS:
            risk_manager_->onDataSourceStatus(static_cast<DataSourceStatusEvent&>(*event));
            break;
        default:
            // Market, Trade, OrderBook events are handled above and fall through here
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
                       (pos.direction == OrderDirection::LONG ? "LONG" : "SHORT"));
            }
        }
        printf("--------------------------\n\n");
    }
}

nlohmann::json Backtester::run_optimization() {
    std::cout << "\n--- RUNNING PARAMETER OPTIMIZATION ---\n" << std::endl;
    
    if (!config_.contains("optimization")) {
        std::cerr << "Optimization requires an 'optimization' section in config.json" << std::endl;
        return nlohmann::json(); // Return an empty json object on error
    }
    auto opt_config = config_["optimization"];
    std::string strategy_to_optimize = opt_config.value("strategy_to_optimize", "");
    if (strategy_to_optimize.empty()) {
        std::cerr << "Optimization config must specify 'strategy_to_optimize'" << std::endl;
        return nlohmann::json(); // Return an empty json object on error
    }

    nlohmann::json base_strategy_config;
    int strategy_idx = -1;
    for (int i = 0; i < config_["strategies"].size(); ++i) {
        if (config_["strategies"][i]["name"] == strategy_to_optimize) {
            base_strategy_config = config_["strategies"][i];
            strategy_idx = i;
            break;
        }
    }
    if (strategy_idx == -1) {
        std::cerr << "Could not find strategy '" << strategy_to_optimize << "' in config." << std::endl;
        return nlohmann::json(); // Return an empty json object on error
    }

    std::vector<nlohmann::json> parameter_sets;
    // This is a simplified grid search. A real implementation might be recursive
    // to handle any number of parameters.
    auto param_ranges = opt_config["param_ranges"];
    for (double p1 = param_ranges["p1_start"]; p1 <= param_ranges["p1_end"]; p1 += param_ranges["p1_step"]) {
        for (double p2 = param_ranges["p2_start"]; p2 <= param_ranges["p2_end"]; p2 += param_ranges["p2_step"]) {
            nlohmann::json params;
            params[param_ranges["p1_name"]] = p1;
            params[param_ranges["p2_name"]] = p2;
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

    // Simple date calculation logic (for production, use a proper date library)
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

        // In-sample period
        auto in_sample_start = current_start_date;
        auto in_sample_end = in_sample_start + std::chrono::hours(24 * in_sample_days);
        
        // Out-of-sample period
        auto out_of_sample_start = in_sample_end;
        auto out_of_sample_end = out_of_sample_start + std::chrono::hours(24 * out_of_sample_days);

        std::cout << "  In-Sample: " << time_to_string(in_sample_start) << " to " << time_to_string(in_sample_end) << std::endl;
        std::cout << "  Out-of-Sample: " << time_to_string(out_of_sample_start) << " to " << time_to_string(out_of_sample_end) << std::endl;
        
        // --- Run in-sample optimization ---
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

        // --- Run out-of-sample backtest with best params ---
        nlohmann::json out_of_sample_config = config_;
        out_of_sample_config["run_mode"] = "BACKTEST";
        out_of_sample_config["data"]["start_date"] = time_to_string(out_of_sample_start);
        out_of_sample_config["data"]["end_date"] = time_to_string(out_of_sample_end);
        
        // Find strategy to optimize and set its params
        std::string strategy_to_optimize = config_["optimization"].value("strategy_to_optimize", "");
        for (int j = 0; j < out_of_sample_config["strategies"].size(); ++j) {
            if (out_of_sample_config["strategies"][j]["name"] == strategy_to_optimize) {
                out_of_sample_config["strategies"][j]["params"] = best_params;
                break;
            }
        }
        
        Backtester out_of_sample_backtester(out_of_sample_config);
        out_of_sample_backtester.run_backtest();
        
        double total_return = out_of_sample_backtester.getPortfolio()->getRealTimePerformance().getTotalReturn();
        out_of_sample_returns.push_back(total_return);
        
        // Move to the next period
        current_start_date = out_of_sample_start;
    }

    // --- Aggregate and Report Results ---
    std::cout << "\n--- Walk-Forward Analysis Results ---\n";
    double total_return_sum = 0;
    for(size_t i = 0; i < out_of_sample_returns.size(); ++i) {
        printf("Split %zu Return: %.2f%%\n", i + 1, out_of_sample_returns[i] * 100);
        total_return_sum += out_of_sample_returns[i];
    }
    double avg_return = total_return_sum / out_of_sample_returns.size();
    printf("\nAverage Out-of-Sample Return: %.2f%%\n", avg_return * 100);
    std::cout << "-------------------------------------\n";
}
