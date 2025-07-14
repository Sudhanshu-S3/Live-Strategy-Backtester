#include <iostream>
#include <memory>
#include <iomanip>
#include <fstream>
#include <stdexcept>
#include <queue>
#include <map>
#include <string>
#include <vector>

#include "../lib/nlohmann/json.hpp"

// Include the new HFT handler
#include "data/HFTDataHandler.h" 
// We no longer need the old handler for this test
// #include "data/HistoricCSVDataHandler.h" 

#include "strategy/Strategy.h"
#include "execution/ExecutionHandler.h"
#include "core/Portfolio.h"
#include "core/Performance.h"
#include "core/Backtester.h"

using json = nlohmann::json;

int main() {
    std::cout << "Starting HFT Data Pipeline Test..." << std::endl;

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
        // 1. Event Queue
        auto event_queue = std::make_shared<std::queue<std::shared_ptr<Event>>>();

        // 2. Data Handler (Using the new HFTDataHandler)
        std::cout << "Initializing HFTDataHandler..." << std::endl;
        auto data_handler = std::make_shared<HFTDataHandler>(
            config["symbols"].get<std::vector<std::string>>(),
            config["data"]["trade_data_dir"].get<std::string>(),
            config["data"]["book_data_dir"].get<std::string>()
        );
        std::cout << "Data loading complete." << std::endl;


        // --- For this initial test, we will bypass the other components ---
        // --- to focus solely on the data handler's output.            ---
        std::shared_ptr<Portfolio> portfolio = nullptr;
        std::shared_ptr<Strategy> strategy = nullptr;
        std::shared_ptr<ExecutionHandler> execution_handler = nullptr;
        // ---

        // 3. Backtester
        // The backtester will now simply run the data handler and process the events
        // using the logging we added to Backtester::run()
        auto backtester = std::make_unique<Backtester>(
            event_queue, data_handler, strategy, portfolio, execution_handler
        );
        
        // 4. Run the backtest
        backtester->run();


    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\nHFT Data Pipeline Test Complete!" << std::endl;
    return 0;
}