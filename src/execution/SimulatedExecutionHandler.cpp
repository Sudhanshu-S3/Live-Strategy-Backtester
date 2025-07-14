#include "../../include/execution/SimulatedExecutionHandler.h"

// --- IMPLEMENT THE NEW CONSTRUCTOR ---
SimulatedExecutionHandler::SimulatedExecutionHandler(
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
    std::shared_ptr<DataHandler> data_handler
) : event_queue(event_queue), data_handler(data_handler) {}


// --- IMPLEMENT THE NEW EVENT-DRIVEN LOGIC ---
void SimulatedExecutionHandler::executeOrder(const OrderEvent& order, const Bar& bar) {
    // Assume fill at the closing price of the provided bar (a simplification).
    double fill_price = bar.close;

    // Simulate a simple commission scheme.
    double commission = 1.99; // Example: $1.99 flat commission per trade

    // Create the FillEvent with the execution details.
    auto fill_event = std::make_shared<FillEvent>(
        bar.timestamp,
        order.symbol,
        "SIMULATED", // Placeholder for the exchange
        order.quantity,
        order.direction,
        fill_price,
        commission
    );

    // Push the FillEvent onto the main event queue for the portfolio to process.
    event_queue->push(fill_event);
};