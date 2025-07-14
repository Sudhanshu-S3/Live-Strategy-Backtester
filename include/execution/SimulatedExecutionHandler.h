#ifndef SIMULATED_EXECUTION_HANDLER_H
#define SIMULATED_EXECUTION_HANDLER_H


#include "ExecutionHandler.h"

// A simple execution handler that simulates trades.

class SimulatedExecutionHandler : public ExecutionHandler {
public:
    // Constructor can take configuration like commission rate.
    SimulatedExecutionHandler(double commission_rate = 0.001);

    // Override the virtual function from the base class.
    FillEvent executeOrder(const OrderEvent& order, const Bar& bar) override;

private:
    double commission_rate;
};

#endif