#ifndef BACKTESTER_H
#define BACKTESTER_H

#include "DataHandler.h"
#include "Strategy.h"
#include "ExecutionHandler.h"
#include "Portfolio.h"
#include "../risk/RishManager.h"
#include <memory>
#include <queue>
#include <string>
#include <chrono>

class Event; // Forward declaration

// STAGE 4: Added RunMode to select between backtesting and live shadow trading
enum class RunMode {
    BACKTEST,
    SHADOW
};

class Backtester {
public:
    Backtester(
        RunMode mode, // STAGE 4: Added mode parameter
        std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
        std::shared_ptr<DataHandler> data_handler,
        std::shared_ptr<Strategy> strategy,
        std::shared_ptr<Portfolio> portfolio,
        std::shared_ptr<ExecutionHandler> execution_handler,
        std::shared_ptr<RiskManager> risk_manager
    );

    void run();
    void compareToBenchmark(const std::string& benchmark_symbol, const std::string& benchmark_filepath);
    void compareLiveToBacktest(const std::string& backtest_results_filepath);

private:
    void handleEvent(const std::shared_ptr<Event>& event);

    RunMode run_mode_; // STAGE 4: Stores the current operating mode

    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue;
    std::shared_ptr<DataHandler> data_handler;
    std::shared_ptr<Strategy> strategy;
    std::shared_ptr<Portfolio> portfolio;
    std::shared_ptr<ExecutionHandler> execution_handler;
    std::shared_ptr<RiskManager> risk_manager;
    bool continue_backtest;

    std::chrono::steady_clock::time_point last_risk_monitor_time_;
    long long risk_monitor_interval_ms_;
};

#endif