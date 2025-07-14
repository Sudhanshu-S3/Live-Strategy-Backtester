#include "../../include/execution/SimulatedExecutionHandler.h"
#include <iostream>
// --- IMPLEMENT THE NEW CONSTRUCTOR ---
SimulatedExecutionHandler::SimulatedExecutionHandler(
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
    std::shared_ptr<DataHandler> data_handler
) : event_queue(event_queue), data_handler(data_handler) {}


// --- IMPLEMENT THE NEW EVENT-DRIVEN LOGIC ---
void SimulatedExecutionHandler::onOrder(const OrderEvent& order) {
    // Get the latest bar for the symbol from the data handler.
    auto bar_optional = data_handler->getLatestBar(order.symbol);

    if (!bar_optional) {
        std::cout << "Warning: Could not get latest bar for " << order.symbol << " to execute order." << std::endl;
        return;
    }
    const Bar& bar = *bar_optional;

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