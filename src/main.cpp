#include <iostream>
#include <memory>
#include <fstream>
#include <stdexcept>
#include <queue>
#include <string>
#include <vector>
#include <thread>

#include "../lib/nlohmann/json.hpp"

#include "data/DataTypes.h"
#include "event/EventQueue.h"
#include "data/HFTDataHandler.h"
#include "data/WebSocketDataHandler.h"
#include "strategy/Strategy.h"
#include "strategy/OrderBookImbalanceStrategy.h"
#include "execution/SimulatedExecutionHandler.h"
#include "core/Portfolio.h"
#include "risk/RiskManager.h"
#include "core/Backtester.h"
#include "core/Performance.h"
#include "analytics/Analytics.h"
#include "core/Optimizer.h"
#include "core/WalkForwardAnalyzer.h"
#include "core/MonteCarloSimulator.h"

using json = nlohmann::json;

// Function to run a single session (backtest or live) and return the portfolio
std::shared_ptr<Portfolio> run_session(const json& config, RunMode mode) {
    try {
        const auto& strategy_config = config["strategies"][0];
        std::string strategy_name = strategy_config["name"];
        
        std::cout << "--- Starting " << (mode == RunMode::BACKTEST ? "BACKTEST" : "LIVE SHADOW") 
                  << " session for strategy '" << strategy_name << "' ---" << std::endl;

        auto event_queue = std::make_shared<EventQueue>();
        
        std::shared_ptr<DataHandler> data_handler;
        if (mode == RunMode::BACKTEST) {
            data_handler = std::make_shared<HFTDataHandler>(
                event_queue,
                config["symbols"].get<std::vector<std::string>>(),
                config["data"]["trade_data_dir"].get<std::string>(),
                config["data"]["book_data_dir"].get<std::string>()
            );
        } else { // SHADOW mode
            data_handler = std::make_shared<WebSocketDataHandler>(
                event_queue,
                config["data"]["websocket_uri"].get<std::string>(),
                config["symbols"].get<std::vector<std::string>>()
            );
        }

        auto portfolio = std::make_shared<Portfolio>(
            event_queue,
            config["initial_capital"],
            data_handler
        );
        auto risk_manager = std::make_shared<RiskManager>(
            *event_queue, *portfolio, strategy_config["risk_per_trade_pct"]
        );
        auto execution_handler = std::make_shared<SimulatedExecutionHandler>(event_queue, data_handler);

        std::shared_ptr<Strategy> strategy;
        if (strategy_name == "ORDER_BOOK_IMBALANCE") {
            auto params = strategy_config["params"];
            strategy = std::make_shared<OrderBookImbalanceStrategy>(
                event_queue, data_handler, strategy_config["symbol"],
                params["lookback_levels"], params["imbalance_threshold"]
            );
        } else {
            throw std::runtime_error("Strategy not recognized: " + strategy_name);
        }

        auto backtester = std::make_unique<Backtester>(
            mode, event_queue, data_handler, strategy, portfolio, execution_handler, risk_manager
        );
        backtester->run();

        std::cout << "--- Session for " << strategy_name << " complete ---" << std::endl;
        portfolio->generateReport();
        return portfolio;

    } catch (const std::exception& e) {
        std::cerr << "Session encountered an error: " << e.what() << std::endl;
        return nullptr;
    }
}

int main() {
    std::cout << "Starting Live vs. Backtest Comparison..." << std::endl;

    json config;
    try {
        std::ifstream config_file("config.json");
        config = json::parse(config_file);
    } catch (const std::exception& e) {
        std::cerr << "Configuration Error: " << e.what() << std::endl;
        return 1;
    }

    std::string run_mode = config.value("run_mode", "BACKTEST");

    if (run_mode == "OPTIMIZATION") {
        Optimizer optimizer(config);
        optimizer.run();
    } else if (run_mode == "WALK_FORWARD") {
        WalkForwardAnalyzer wf_analyzer(config);
        wf_analyzer.run();
    } else if (run_mode == "MONTE_CARLO") {
        MonteCarloSimulator mc_simulator(config);
        mc_simulator.run(config["monte_carlo"].value("num_simulations", 100));
    } else {
        if (!config.contains("strategies") || !config["strategies"].is_array() || config["strategies"].empty()) {
            std::cerr << "Config error: 'strategies' must be an array with at least one strategy." << std::endl;
            return 1;
        }

        // Run backtest session
        auto backtest_portfolio = run_session(config, RunMode::BACKTEST);

        // Run live shadow session
        auto live_portfolio = run_session(config, RunMode::SHADOW);

        // Compare performance
        if (backtest_portfolio && live_portfolio) {
            Analytics analytics(config.contains("analytics") ? config["analytics"] : json({}));
            analytics.comparePerformance(live_portfolio, backtest_portfolio);
        } else {
            std::cerr << "Could not perform comparison due to errors in one or both sessions." << std::endl;
        }
    }

    std::cout << "\nComparison complete!" << std::endl;
    return 0;
}