#ifndef SIMULATED_EXECUTION_HANDLER_H
#define SIMULATED_EXECUTION_HANDLER_H

#include "ExecutionHandler.h"
#include "../event/ThreadSafeQueue.h"
#include "../data/DataHandler.h"

class SimulatedExecutionHandler : public ExecutionHandler {
private:
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue_;
    std::shared_ptr<DataHandler> data_handler_;

public:
    SimulatedExecutionHandler(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
                              std::shared_ptr<DataHandler> data_handler);

    void onOrder(const OrderEvent& order) override;
};

#endif // SIMULATED_EXECUTION_HANDLER_H