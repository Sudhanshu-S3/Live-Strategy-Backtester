#ifndef SIMULATED_EXECUTION_HANDLER_H
#define SIMULATED_EXECUTION_HANDLER_H

#include <memory>
#include "core/Portfolio.h"
#include "data/DataHandler.h"
#include "data/HFTDataHandler.h" // 1. Include the HFTDataHandler header
#include "event/Event.h"
#include "event/EventQueue.h"

class SimulatedExecutionHandler {
public:
    /**
     * @brief Construct a new Simulated Execution Handler object
     * * @param events Reference to the master event queue
     * @param data_handler Reference to the HFT data handler to access live order books
     */
    // 2. Update the constructor to accept a reference to HFTDataHandler
    SimulatedExecutionHandler(EventQueue& events, HFTDataHandler& data_handler);

    /**
     * @brief Processes an OrderEvent to simulate its execution against the order book
     * * @param order_event The order to be executed
     */
    void executeOrder(const OrderEvent& order_event);

private:
    EventQueue& events_;
    // 3. Add a member variable to store the reference to the data handler
    HFTDataHandler& data_handler_; 
};

#endif // SIMULATED_EXECUTION_HANDLER_H