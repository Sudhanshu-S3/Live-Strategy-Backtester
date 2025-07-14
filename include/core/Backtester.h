#ifndef BACKTESTER_H
#define BACKTESTER_H

#include "DataHandler.h"
#include "Strategy.h"
#include "ExecutionHandler.h"
#include "Portfolio.h" 
#include <memory>

using namespace std;

class Backtester {
public:
    
    Backtester(
        unique_ptr<DataHandler> data_handler, 
        unique_ptr<Strategy> strategy,
        unique_ptr<ExecutionHandler> execution_handler,
        Portfolio* portfolio 
    );

    
    void run();

private:
    unique_ptr<DataHandler> data_handler;
    unique_ptr<Strategy> strategy;
    unique_ptr<ExecutionHandler> execution_handler;
    Portfolio* portfolio; 
};

#endif