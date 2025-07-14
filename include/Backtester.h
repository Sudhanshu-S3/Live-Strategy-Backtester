#ifndef BACKTESTER_H
#define BACKTESTER_H

#include "DataHandler.h"
#include "Strategy.h"
#include "ExecutionHandler.h" // New include
#include <memory>

using namespace std;

// The main backtesting engine.
class Backtester {

private:
    unique_ptr<DataHandler> data_handler;
    unique_ptr<Strategy> strategy;
    unique_ptr<ExecutionHandler> execution_handler;
    
public:
    // Constructor now takes an ExecutionHandler as well.
    Backtester(
        unique_ptr<DataHandler> data_handler, 
        unique_ptr<Strategy> strategy,
        unique_ptr<ExecutionHandler> execution_handler
    );

    // Runs the backtest event loop.
    void run();

 // New member
};

#endif 