#include "../../include/execution/SimulatedExecutionHandler.h"
#include <iostream>
#include <algorithm> // For std::min

/**
 * @brief Construct a new Simulated Execution Handler object
 * Stores references to the event queue and the HFT data handler.
 */
SimulatedExecutionHandler::SimulatedExecutionHandler(EventQueue& events, HFTDataHandler& data_handler)
    : events_(events), data_handler_(data_handler) {}

/**
 * @brief Processes an OrderEvent to simulate its execution against the order book.
 * This is the core "walk the book" logic for slippage simulation.
 */
void SimulatedExecutionHandler::executeOrder(const OrderEvent& order_event) {
    // We only simulate slippage for MARKET orders.
    // LIMIT order execution is a different, more complex simulation.
    if (order_event.type != OrderType::MARKET) {
        return;
    }

    // 1. Get the most recent order book for the symbol from the data handler.
    const OrderBook& book = data_handler_.get_latest_book(order_event.symbol);

    double quantity_to_fill = order_event.quantity;
    double total_cost = 0.0;
    double quantity_filled = 0.0;

    if (order_event.direction == OrderDirection::BUY) {
        // For a BUY order, we "walk the asks", starting from the best (lowest) price.
        for (const auto& level : book.asks) {
            double price = level.price;
            double available_quantity = level.quantity;

            // Determine the amount we can trade at this level
            double trade_quantity = std::min(quantity_to_fill, available_quantity);

            // Update our metrics
            total_cost += trade_quantity * price;
            quantity_filled += trade_quantity;
            quantity_to_fill -= trade_quantity;

            // If the order is fully filled, stop walking the book
            if (quantity_to_fill <= 1e-9) { // Use a small epsilon for float comparison
                break;
            }
        }
    } else { // OrderDirection::SELL
        // For a SELL order, we "walk the bids", starting from the best (highest) price.
        for (const auto& level : book.bids) {
            double price = level.price;
            double available_quantity = level.quantity;

            // Determine the amount we can trade at this level
            double trade_quantity = std::min(quantity_to_fill, available_quantity);

            // Update our metrics
            total_cost += trade_quantity * price;
            quantity_filled += trade_quantity;
            quantity_to_fill -= trade_quantity;

            // If the order is fully filled, stop walking the book
            if (quantity_to_fill <= 1e-9) { // Use a small epsilon for float comparison
                break;
            }
        }
    }

    // 2. If any part of the order was filled, calculate the VWAP and create a FillEvent.
    if (quantity_filled > 0) {
        double vwap = total_cost / quantity_filled;

        // Here we can also add a commission model if desired
        double commission = 0.0; // Placeholder for a future commission model

        FillEvent fill_event(
            order_event.timestamp,
            order_event.symbol,
            "SIMULATED_EXCHANGE",
            order_event.direction,
            quantity_filled,
            vwap,
            commission
        );

        // Push the FillEvent to the main queue for the Portfolio to process.
        events_.push(std::make_unique<FillEvent>(fill_event));

        // Optional: Log partial fills
        if (quantity_to_fill > 1e-9) {
            std::cout << "WARNING: Partial fill for order on " << order_event.symbol
                      << ". Requested: " << order_event.quantity
                      << ", Filled: " << quantity_filled << std::endl;
        }
    } else {
        std::cout << "WARNING: Order for " << order_event.quantity << " of " << order_event.symbol 
                  << " could not be filled (insufficient liquidity)." << std::endl;
    }
}