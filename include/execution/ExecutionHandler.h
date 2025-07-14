#ifndef EXECUTION_HANDLER_H
#define EXECUTION_HANDLER_H

#include "../data/DataTypes.h" // Corrected include path
#include <memory> // Required for shared_ptr
#include <queue>  // Required for queue

class ExecutionHandler {
public:
    virtual ~ExecutionHandler() = default;

    // The backtester will call this method when it receives an OrderEvent.
    // The implementation (e.g., SimulatedExecutionHandler) will process the order
    // and generate a FillEvent.
    virtual void onOrder(const OrderEvent& order) = 0;
};

#endif // EXECUTION_HANDLER_H