#include <iostream>
#include <memory>
#include <iomanip>
#include <fstream>
#include <stdexcept>
#include <queue>
#include <map>
#include <string>

#include "../lib/nlohmann/json.hpp"

// Standardized include paths
#include "../include/data/DataHandler.h"
#include "../include/data/HistoricCSVDataHandler.h"
#include "../include/strategy/Strategy.h"
#include "../include/strategy/PairTradingStrategy.h"
#include "../include/execution/ExecutionHandler.h"
#include "../include/execution/SimulatedExecutionHandler.h"
#include "../include/core/Portfolio.h"
#include "../include/core/Performance.h"
#include "../include/core/Backtester.h"

using json = nlohmann::json;

int main() {
    std::cout << "Starting Event-Driven Multi-Asset Backtest..." << std::endl;

    json config;
    try {
        std::ifstream config_file("config.json");
        if (!config_file.is_open()) throw std::runtime_error("Could not open config.json");
        config = json::parse(config_file);
    } catch (const std::exception& e) {
        std::cerr << "Configuration Error: " << e.what() << std::endl;
        return 1;
    }

    try {
        auto event_queue = std::make_shared<std::queue<std::shared_ptr<Event>>>();

        auto data_handler = std::make_shared<HistoricCSVDataHandler>(
            config["data"]["symbol_filepaths"].get<std::map<std::string, std::string>>()
        );

        auto portfolio = std::make_shared<Portfolio>(config["initial_capital"], data_handler);
        auto execution_handler = std::make_shared<SimulatedExecutionHandler>(event_queue, data_handler);

        std::shared_ptr<Strategy> strategy;
        std::string strategy_name = config["strategy"]["name"];
        std::cout << "Using strategy: " << strategy_name << std::endl;

        if (strategy_name == "PAIR_TRADING") {
            auto params = config["strategy"]["params"];
            strategy = std::make_shared<PairTradingStrategy>(
                params["symbol_a"], params["symbol_b"], params["window"],
                params["z_score_threshold"], data_handler
            );
        } else {
            throw std::runtime_error("Strategy not recognized: " + strategy_name);
        }

        auto backtester = std::make_unique<Backtester>(
            event_queue, data_handler, strategy, portfolio, execution_handler
        );
        
        backtester->run();

        std::cout << "\n<><><><><><><> PERFORMANCE REPORT <><><><><><><>\n" << std::endl;
        
        double initial_capital = config["initial_capital"];
        const auto& equity_curve = portfolio->getEquityCurve();
        
        if (equity_curve.empty()) {
            std::cout << "No trading activity. Performance metrics are not applicable." << std::endl;
        } else {
            double final_value = equity_curve.back();
            Performance perf(equity_curve, initial_capital);
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "--- OVERVIEW ---\n"
                      << "Initial Capital:       $" << initial_capital << "\n"
                      << "Final Portfolio Value: $" << final_value << "\n\n"
                      << "--- KEY METRICS ---\n"
                      << "Total Return:          " << perf.getTotalReturn() * 100.0 << "%\n"
                      << "Max Drawdown:          " << perf.getMaxDrawdown() * 100.0 << "%\n"
                      << "Annualized Sharpe Ratio: " << perf.getSharpeRatio() << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\nBacktest complete!" << std::endl;
    return 0;
}