#ifndef BACKTESTER_H
#define BACKTESTER_H

#include "DataHandler.h"
#include "Strategy.h"
#include <memory>

using namespace std;

// Main backtesting engine.
class Backtester {
public:
    // Constructor for DataHandler and a Strategy.
    Backtester(unique_ptr<DataHandler> data_handler, unique_ptr<Strategy> strategy);

    void run();

private:
    unique_ptr<DataHandler> data_handler;
    unique_ptr<Strategy> strategy;
};

#endif