#ifndef EXECUTION_HANDLER_H
#define EXECUTION_HANDLER_H

#include <memory>
#include "../event/Event.h" // <-- ADD THIS LINE

// Abstract base class for handling order execution.
// This can be a live connection to a brokerage or a simulator.
class ExecutionHandler {
public:
    virtual ~ExecutionHandler() = default;

    // Pure virtual function to be implemented by derived classes.
    // It takes a constant reference to an OrderEvent.
    virtual void onOrder(const OrderEvent& order) = 0;
};

#endif // EXECUTION_HANDLER_H