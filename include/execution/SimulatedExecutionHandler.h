#ifndef SIMULATED_EXECUTION_HANDLER_H
#define SIMULATED_EXECUTION_HANDLER_H

#include "ExecutionHandler.h"
#include "../data/DataHandler.h" // Include DataHandler
#include <queue>
#include <memory>

class SimulatedExecutionHandler : public ExecutionHandler {
public:
    // --- CONSTRUCTOR UPDATED ---
    // Now takes both the event queue and the data handler.
    SimulatedExecutionHandler(
        std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
        std::shared_ptr<DataHandler> data_handler
    );

    // The method signature is simple: it receives an order and processes it.
    void onOrder(const OrderEvent& order) override;

private:
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue;
    std::shared_ptr<DataHandler> data_handler;
};

#endif // SIMULATED_EXECUTION_HANDLER_H