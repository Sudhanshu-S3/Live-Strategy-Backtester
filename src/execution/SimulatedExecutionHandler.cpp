#include "../../include/execution/SimulatedExecutionHandler.h"
#include <iostream>
#include <algorithm>

SimulatedExecutionHandler::SimulatedExecutionHandler(
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
    std::shared_ptr<DataHandler> data_handler
) : event_queue_(event_queue), data_handler_(data_handler) {}

void SimulatedExecutionHandler::onOrder(const OrderEvent& order_event) {
    // For both backtesting and shadow mode, we simulate the execution
    // to generate a FillEvent for the portfolio.
    if (order_event.type != OrderType::MARKET) {
        std::cout << "SimulatedExecutionHandler only supports MARKET orders." << std::endl;
        return;
    }

    // This is a simplified execution model. A real one would get the latest order book.
    // For now, we use the latest trade price as the fill price (no slippage).
    double fill_price = data_handler_->getLatestBarValue(order_event.symbol, "price");
    
    if (fill_price <= 0) {
        std::cerr << "Execution Warning: Could not get a valid fill price for " << order_event.symbol 
                  << ". Order not filled." << std::endl;
        return;
    }

    double commission = 0.0; // Placeholder for commission model

    auto fill_event = std::make_shared<FillEvent>(
        order_event.timestamp,
        order_event.symbol,
        "SIMULATED_EXCHANGE",
        order_event.direction,
        order_event.quantity,
        fill_price,
        commission
    );

    event_queue_->push(fill_event);
};