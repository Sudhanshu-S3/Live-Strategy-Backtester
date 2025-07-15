#ifndef SIMULATED_EXECUTION_HANDLER_H
#define SIMULATED_EXECUTION_HANDLER_H

#include "ExecutionHandler.h"
#include "../data/DataHandler.h"
#include "../event/ThreadSafeQueue.h"
#include "../event/Event.h"
#include <memory>

class SimulatedExecutionHandler : public ExecutionHandler {
public:
    SimulatedExecutionHandler(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue, 
                              std::shared_ptr<DataHandler> data_handler);

    void onOrder(const OrderEvent& order) override;

private:
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue_;
    std::shared_ptr<DataHandler> data_handler_; 
};

#endif // SIMULATED_EXECUTION_HANDLER_H