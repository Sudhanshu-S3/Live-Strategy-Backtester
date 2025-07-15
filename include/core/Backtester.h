#ifndef BACKTESTER_H
#define BACKTESTER_H

#include "DataHandler.h"
#include "Strategy.h"
#include "ExecutionHandler.h"
#include "Portfolio.h"
#include "../risk/RishManager.h" // Corrected path
#include <memory>
#include <queue>

class Event;

class Backtester {
public:
    Backtester(
        std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
        std::shared_ptr<DataHandler> data_handler,
        std::shared_ptr<Strategy> strategy,
        std::shared_ptr<Portfolio> portfolio,
        std::shared_ptr<ExecutionHandler> execution_handler,
        std::shared_ptr<RiskManager> risk_manager // Added risk manager
    );
    
    void run();

private:
    void handleEvent(const std::shared_ptr<Event>& event);

    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue;
    std::shared_ptr<DataHandler> data_handler;
    std::shared_ptr<Strategy> strategy;
    std::shared_ptr<Portfolio> portfolio;
    std::shared_ptr<ExecutionHandler> execution_handler;
    std::shared_ptr<RiskManager> risk_manager; // Added risk manager
    bool continue_backtest;
};

#endif