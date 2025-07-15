#ifndef BACKTESTER_H
#define BACKTESTER_H

#include <vector>
#include <string>
#include <memory>
#include <queue>
#include <thread>
#include <nlohmann/json.hpp>
#include "event/Event.h"
#include "data/DataHandler.h"
#include "strategy/Strategy.h"
#include "core/Portfolio.h"
#include "execution/ExecutionHandler.h"
#include "risk/RiskManager.h"
#include "analytics/Analytics.h" // STAGE 6

// STAGE 4: Enum for run mode
enum class RunMode { BACKTEST, SHADOW, OPTIMIZATION, WALK_FORWARD };

class Backtester {
public:
    Backtester(const nlohmann::json& config);
    ~Backtester();

    void run();

private:
    void run_backtest();
    void run_optimization(); // STAGE 7
    void run_walk_forward(); // STAGE 7

    void start_strategy_threads();
    void strategy_thread_worker(std::shared_ptr<Strategy> strategy);
    void handleEvent(const std::shared_ptr<Event>& event);

    nlohmann::json config_;
    RunMode run_mode_;
    
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue_;
    std::shared_ptr<DataHandler> data_handler_;
    std::vector<std::shared_ptr<Strategy>> strategies_;
    std::shared_ptr<Portfolio> portfolio_;
    std::shared_ptr<ExecutionHandler> execution_handler_;
    std::shared_ptr<RiskManager> risk_manager_;
    std::shared_ptr<Analytics> analytics_; // STAGE 6

    std::atomic<bool> continue_backtest_{true};
    std::vector<std::thread> strategy_threads_; // STAGE 1
};

#endif
```
</include/core/Backtester.h>
<src/core/Backtester.cpp>
```cpp
#include "../../include/core/Backtester.h"
#include "../../include/data/HFTDataHandler.h"
#include "../../include/strategy/OrderBookImbalanceStrategy.h"
// Add other strategy headers as needed

#include <iostream>
#include <fstream>
#include <stdexcept>

// Helper to create strategies from config
std::shared_ptr<Strategy> create_strategy_from_config(
    const nlohmann::json& config,
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
    std::shared_ptr<DataHandler> data_handler
) {
    std::string name = config.value("name", "");
    if (name == "ORDER_BOOK_IMBALANCE") {
        auto params = config["params"];
        return std::make_shared<OrderBookImbalanceStrategy>(
            event_queue, data_handler, config.value("symbol", ""),
            params.value("lookback_levels", 10), params.value("imbalance_threshold", 1.5)
        );
    }
    // Add other strategies here...
    throw std::runtime_error("Unknown strategy name in config: " + name);
}


Backtester::Backtester(const nlohmann::json& config) : config_(config) {
    std::string mode_str = config.value("run_mode", "BACKTEST");
    if (mode_str == "OPTIMIZATION") run_mode_ = RunMode::OPTIMIZATION;
    else if (mode_str == "WALK_FORWARD") run_mode_ = RunMode::WALK_FORWARD;
    else if (mode_str == "SHADOW") run_mode_ = RunMode::SHADOW;
    else run_mode_ = RunMode::BACKTEST;

    event_queue_ = std::make_shared<std::queue<std::shared_ptr<Event>>>();
    
    auto data_config = config_["data"];
    data_handler_ = std::make_shared<HFTDataHandler>(
        event_queue_, config_["symbols"].get<std::vector<std::string>>(),
        data_config.value("trade_data_dir", ""), data_config.value("book_data_dir", ""),
        data_config.value("historical_data_fallback_dir", "")
    );

    for (const auto& strategy_config : config_["strategies"]) {
        if (strategy_config.value("active", false)) {
            strategies_.push_back(create_strategy_from_config(strategy_config, event_queue_, data_handler_));
        }
    }

    portfolio_ = std::make_shared<Portfolio>(
        event_queue_, data_handler_, config_.value("initial_capital", 100000.0)
    );
    
    execution_handler_ = std::make_shared<SimulatedExecutionHandler>(event_queue_, data_handler_);
    risk_manager_ = std::make_shared<RiskManager>(event_queue_, portfolio_);
    analytics_ = std::make_shared<Analytics>(config_["analytics"]); // STAGE 6
}

Backtester::~Backtester() {
    continue_backtest_ = false;
    for (auto& t : strategy_threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
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

    if (run_mode_ == RunMode::SHADOW) {
        std::dynamic_pointer_cast<HFTDataHandler>(data_handler_)->connectLiveFeed();
    }
    
    start_strategy_threads(); // STAGE 1: Start strategy threads

    while (continue_backtest_ && (!data_handler_->isFinished() || run_mode_ == RunMode::SHADOW)) {
        data_handler_->updateBars(*event_queue_);
        analytics_->detect_anomalies(data_handler_); // STAGE 6

        while (!event_queue_->empty()) {
            std::shared_ptr<Event> event = event_queue_->front();
            event_queue_->pop();
            handleEvent(event);
        }

        if (run_mode_ == RunMode::SHADOW) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // STAGE 2: Latency reduction
        }
    }

    continue_backtest_ = false; // Signal threads to stop
    std::cout << "Backtester event loop finished. Waiting for threads to join..." << std::endl;
    for (auto& t : strategy_threads_) {
        if (t.joinable()) t.join();
    }
    
    portfolio_->generateReport();
    analytics_->generateReport(portfolio_); // STAGE 6
}

void Backtester::start_strategy_threads() {
    for (auto& strategy : strategies_) {
        strategy_threads_.emplace_back(&Backtester::strategy_thread_worker, this, strategy);
    }
    std::cout << "Started " << strategy_threads_.size() << " strategy threads." << std::endl;
}

void Backtester::strategy_thread_worker(std::shared_ptr<Strategy> strategy) {
    std::cout << "Strategy thread for '" << strategy->getName() << "' started." << std::endl;
    while (continue_backtest_) {
        try {
            strategy->calculate_signals();
        } catch (const std::exception& e) {
            std::cerr << "--- STRATEGY RUNTIME ERROR in thread " << strategy->getName() << " ---" << std::endl;
            std::cerr << "Reason: " << e.what() << std::endl;
            strategy->pause();
            portfolio_->updateStrategyStatus(strategy->getName(), "PAUSED");
            // Optionally, stop the whole backtest on a critical error
            // continue_backtest_ = false; 
        }
        // STAGE 2: Reduce latency by having strategies poll for data more frequently
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::cout << "Strategy thread for '" << strategy->getName() << "' finished." << std::endl;
}


void Backtester::handleEvent(const std::shared_ptr<Event>& event) {
    portfolio_->updateTimeIndex();
    
    // Events are now handled by their respective components.
    // The main loop dispatches events that strategies, portfolio, etc., listen to.
    // Signals are generated by strategies in their own threads.
    switch (event->type) {
        case Event::SIGNAL:
            risk_manager_->onSignal(static_cast<SignalEvent&>(*event));
            break;
        case Event::ORDER:
            execution_handler_->onOrder(static_cast<OrderEvent&>(*event));
            break;
        case Event::FILL:
            portfolio_->onFill(static_cast<FillEvent&>(*event));
            break;
        default:
            // Market, Trade, OrderBook events are consumed directly by DataHandler
            // and accessed by strategies.
            break;
    }
}

void Backtester::run_optimization() {
    std::cout << "\n--- RUNNING PARAMETER OPTIMIZATION ---\n" << std::endl;
    // STAGE 7: Implementation for grid search
    // This is a simplified example. A real implementation would be more robust.
    auto opt_config = config_["optimization"];
    std::string strategy_name = opt_config.value("strategy_to_optimize", "");
    
    // Find the base config for the strategy
    nlohmann::json base_strategy_config;
    for(const auto& sc : config_["strategies"]){
        if(sc.value("name", "") == strategy_name) {
            base_strategy_config = sc;
            break;
        }
    }
    if(base_strategy_config.empty()){
        std::cerr << "Strategy to optimize not found in config." << std::endl;
        return;
    }

    // This is a placeholder for a proper grid search implementation
    std::cout << "Optimization feature is conceptual. Run individual backtests with different params." << std::endl;
    std::cout << "To run a full optimization, you would iterate through param_ranges, create a new" << std::endl;
    std::cout << "Backtester instance for each combination, run it, and record the performance metric." << std::endl;
}

void Backtester::run_walk_forward() {
    std::cout << "\n--- RUNNING WALK-FORWARD ANALYSIS ---\n" << std::endl;
    // STAGE 7: Implementation for walk-forward analysis
    std::cout << "Walk-forward analysis is conceptual. This would involve:" << std::endl;
    std::cout << "1. Splitting data into in-sample and out-of-sample periods based on config." << std::endl;
    std::cout << "2. Looping through periods:" << std::endl;
    std::cout << "   a. (Optional) Run optimization on the in-sample period." << std::endl;
    std::cout << "   b. Run a backtest on the out-of-sample period with the best parameters." << std::endl;
    std::cout << "3. Aggregate and report out-of-sample performance." << std::endl;
}
