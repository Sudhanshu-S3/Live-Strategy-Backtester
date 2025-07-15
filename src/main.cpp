#include <iostream>
#include <memory>
#include <fstream>
#include <stdexcept>
#include <queue>
#include <string>
#include <vector>

#include "../lib/nlohmann/json.hpp"

#include "data/DataTypes.h"
#include "event/EventQueue.h"
#include "data/HFTDataHandler.h"
#include "strategy/Strategy.h"
#include "strategy/OrderBookImbalanceStrategy.h"
#include "execution/SimulatedExecutionHandler.h"
#include "core/Portfolio.h"
#include "risk/RishManager.h" // Corrected include path
#include "core/Backtester.h"

using json = nlohmann::json;

int main() {
    std::cout << "Starting Backtester..." << std::endl;

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
        // 1. Event Queue
        auto event_queue = std::make_shared<EventQueue>();

        // 2. Data Handler
        auto data_handler = std::make_shared<HFTDataHandler>(
            event_queue,
            config["symbols"].get<std::vector<std::string>>(),
            config["data"]["trade_data_dir"].get<std::string>(),
            config["data"]["book_data_dir"].get<std::string>()
        );

        // 3. Portfolio
        auto portfolio = std::make_shared<Portfolio>(
            event_queue,
            config["initial_capital"],
            data_handler
        );

        // 4. Risk Manager
        auto risk_manager = std::make_shared<RiskManager>(
            *event_queue, *portfolio, config["strategy"]["risk_per_trade_pct"]
        );

        // 5. Execution Handler
        auto execution_handler = std::make_shared<SimulatedExecutionHandler>(event_queue, data_handler);

        // 6. Strategy
        std::shared_ptr<Strategy> strategy;
        std::string strategy_name = config["strategy"]["name"];
        std::cout << "Using strategy: " << strategy_name << std::endl;

        if (strategy_name == "ORDER_BOOK_IMBALANCE") {
            auto params = config["strategy"]["params"];
            strategy = std::make_shared<OrderBookImbalanceStrategy>(
                event_queue,
                data_handler,
                config["strategy"]["symbol"],
                params["lookback_levels"],
                params["imbalance_threshold"]
            );
        } else {
            throw std::runtime_error("Strategy not recognized: " + strategy_name);
        }

        // 7. Backtester
        auto backtester = std::make_unique<Backtester>(
            event_queue,
            data_handler,
            strategy,
            portfolio,
            execution_handler,
            risk_manager
        );
        
        // 8. Run the backtest
        std::cout << "Running backtest..." << std::endl;
        backtester->run();
        std::cout << "Backtest finished." << std::endl;

        // 9. Generate and Save Performance Report
        std::cout << "\n<><><><><><><> PERFORMANCE REPORT <><><><><><><>\n" << std::endl;
        portfolio->generateReport();
        portfolio->writeResultsToCSV("performance_report.csv");
        std::cout << "Performance report saved to performance_report.csv" << std::endl;


    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\nBacktester Complete!" << std::endl;
    return 0;
}