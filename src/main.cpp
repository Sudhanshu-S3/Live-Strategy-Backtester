#define _HAS_STD_BYTE 0
#include <iostream>
#include <memory>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "../include/core/Backtester.h"
#include "../include/core/Portfolio.h"
#include "../include/analytics/Analytics.h"
#include "../include/core/Optimizer.h"
#include "../include/core/WalkForwardAnalyzer.h"
#include "../include/core/MonteCarloSimulator.h"

using json = nlohmann::json;

// Function to run a single session (backtest or live) and return the portfolio
std::shared_ptr<Portfolio> run_session(const json& original_config, RunMode mode) {
    try {
        std::cout << "--- Starting " << (mode == RunMode::BACKTEST ? "BACKTEST" : "SHADOW") 
                  << " session ---" << std::endl;

        // Create a mutable copy of the config to set the run_mode
        json session_config = original_config;
        if (mode == RunMode::BACKTEST) {
            session_config["run_mode"] = "BACKTEST";
        } else if (mode == RunMode::SHADOW) {
            session_config["run_mode"] = "SHADOW";
        }

        // The Backtester is designed to be constructed with a config
        // and will create all the necessary components internally.
        Backtester backtester(session_config);
        backtester.run();

        std::cout << "--- Session complete ---" << std::endl;
        
        auto portfolio = backtester.getPortfolio();
        if (portfolio) {
            portfolio->generateReport();
        }
        
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
        if (!config_file.is_open()) {
            throw std::runtime_error("Could not open config.json");
        }
        config = json::parse(config_file);
    } catch (const std::exception& e) {
        std::cerr << "Configuration Error: " << e.what() << std::endl;
        return 1;
    }

    try {
        std::string run_mode_str = config.value("run_mode", "BACKTEST");

        if (run_mode_str == "OPTIMIZATION") {
            Optimizer optimizer(config);
            optimizer.run();
        } else if (run_mode_str == "WALK_FORWARD") {
            WalkForwardAnalyzer wf_analyzer(config);
            wf_analyzer.run();
        } else if (run_mode_str == "MONTE_CARLO") {
            MonteCarloSimulator mc_simulator(config);
            mc_simulator.run(config["monte_carlo"].value("num_simulations", 1000));
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
    } catch (const std::exception& e) {
        std::cerr << "Error during processing: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\nComparison complete!" << std::endl;
    return 0;
}