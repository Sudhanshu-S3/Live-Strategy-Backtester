#include <iostream>
#include <memory>
#include <fstream>
#include <stdexcept>
#include <queue>
#include <string>
#include <vector>
#include <thread> // STAGE 1: Required for multi-threading

#include "../lib/nlohmann/json.hpp"

#include "data/DataTypes.h"
#include "event/EventQueue.h"
#include "data/HFTDataHandler.h"
#include "strategy/Strategy.h"
#include "strategy/OrderBookImbalanceStrategy.h"
#include "execution/SimulatedExecutionHandler.h"
#include "core/Portfolio.h"
#include "risk/RishManager.h" 
#include "core/Backtester.h"
#include "core/Performance.h"

using json = nlohmann::json;

// STAGE 1: Encapsulates the logic for a single backtest instance to be run in a thread
void run_backtest_instance(const json& strategy_config, const json& global_config) {
    try {
        std::string strategy_name = strategy_config["name"];
        std::cout << "Thread " << std::this_thread::get_id() << ": Starting backtest for strategy '" << strategy_name << "'..." << std::endl;

        RunMode mode = RunMode::BACKTEST;
        if (global_config.contains("run_mode") && global_config["run_mode"] == "SHADOW") {
            mode = RunMode::SHADOW;
        }

        // Create an isolated environment for this thread
        auto event_queue = std::make_shared<EventQueue>();
        auto data_handler = std::make_shared<HFTDataHandler>(
            event_queue,
            global_config["symbols"].get<std::vector<std::string>>(),
            global_config["data"]["trade_data_dir"].get<std::string>(),
            global_config["data"]["book_data_dir"].get<std::string>()
        );
        auto portfolio = std::make_shared<Portfolio>(
            event_queue,
            global_config["initial_capital"],
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
                event_queue,
                data_handler,
                strategy_config["symbol"],
                params["lookback_levels"],
                params["imbalance_threshold"]
            );
        } else {
            throw std::runtime_error("Strategy not recognized: " + strategy_name);
        }

        auto backtester = std::make_unique<Backtester>(
            mode, event_queue, data_handler, strategy, portfolio, execution_handler, risk_manager
        );

        backtester->run();

        // Generate report for this specific instance
        std::string report_filename = "performance_report_" + strategy_name + ".csv";
        std::cout << "\n<><><><><><><> PERFORMANCE REPORT FOR " << strategy_name << " <><><><><><><>\n" << std::endl;
        portfolio->generateReport();
        portfolio->writeResultsToCSV(report_filename);
        std::cout << "Performance report for " << strategy_name << " saved to " << report_filename << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Thread " << std::this_thread::get_id() << " encountered an error: " << e.what() << std::endl;
    }
}


int main() {
    std::cout << "Starting Parallel Backtester..." << std::endl;

    json config;
    try {
        std::ifstream config_file("config.json");
        config = json::parse(config_file);
    } catch (const std::exception& e) {
        std::cerr << "Configuration Error: " << e.what() << std::endl;
        return 1;
    }

    // STAGE 1: Launch a thread for each strategy in the config
    std::vector<std::thread> backtest_threads;
    if (config.contains("strategies") && config["strategies"].is_array()) {
        for (const auto& strategy_config : config["strategies"]) {
            backtest_threads.emplace_back(run_backtest_instance, strategy_config, config);
        }
    } else {
        std::cerr << "Config error: 'strategies' field must be an array of strategy configurations." << std::endl;
        return 1;
    }

    // Wait for all backtest threads to complete
    for (auto& t : backtest_threads) {
        t.join();
    }

    std::cout << "\nAll backtests complete!" << std::endl;
    return 0;
}