#include "../../include/execution/SimulatedExecutionHandler.h"
#include <iostream>
#include <memory>
#include <numeric>

// Update the constructor signature to match the corrected header
SimulatedExecutionHandler::SimulatedExecutionHandler(
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
    std::shared_ptr<DataHandler> data_handler
) : event_queue_(event_queue), data_handler_(data_handler) {}

void SimulatedExecutionHandler::onOrder(const OrderEvent& order_event) {
    // A simple simulation: assume the order is filled immediately at the last known price.
    auto latest_bar = data_handler_->getLatestBar(order_event.symbol);
    if (latest_bar) {
        double fill_price = latest_bar->close;
        double commission = 0.0; // Simplified

        auto fill_event = std::make_shared<FillEvent>(
            order_event.timestamp,
            order_event.symbol,
            order_event.strategy_name,
            order_event.direction,
            order_event.quantity,
            fill_price,
            commission
        );
        // The queue expects a std::shared_ptr<std::shared_ptr<Event>>, so we must wrap the event pointer.
        event_queue_->push(std::make_shared<std::shared_ptr<Event>>(fill_event));
    } else {
        std::cerr << "SimulatedExecutionHandler: Could not get latest bar for " << order_event.symbol << " to fill order." << std::endl;
    }
}