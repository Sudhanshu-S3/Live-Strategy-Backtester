#include "../../include/execution/SimulatedExecutionHandler.h"
#include <iostream>
#include <memory>
#include <numeric>

SimulatedExecutionHandler::SimulatedExecutionHandler(
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
    std::shared_ptr<DataHandler> data_handler
) : event_queue_(event_queue), data_handler_(data_handler) {}

void SimulatedExecutionHandler::onOrder(const OrderEvent& order_event) {
    if (order_event.type != OrderType::MARKET) {
        std::cout << "SimulatedExecutionHandler only supports MARKET orders." << std::endl;
        return;
    }

    auto book_opt = data_handler_->getLatestOrderBook(order_event.symbol);
    if (!book_opt) {
        std::cerr << "Execution Error: No order book data available for " << order_event.symbol 
                  << ". Order not filled." << std::endl;
        return;
    }
    const OrderBook& book = *book_opt;

    double quantity_to_fill = order_event.quantity;
    double total_cost = 0.0;
    double quantity_filled = 0.0;

    if (order_event.direction == OrderDirection::BUY) {
        if (book.asks.empty()) {
            std::cerr << "Execution Warning: No asks on order book for " << order_event.symbol << ". Cannot execute BUY order." << std::endl;
            return;
        }
        for (const auto& level : book.asks) {
            double price = level.first;
            double available_quantity = level.second;
            double trade_quantity = std::min(quantity_to_fill, available_quantity);

            total_cost += trade_quantity * price;
            quantity_filled += trade_quantity;
            quantity_to_fill -= trade_quantity;

            if (quantity_to_fill < 1e-9) break;
        }
    } else { // SELL
        if (book.bids.empty()) {
            std::cerr << "Execution Warning: No bids on order book for " << order_event.symbol << ". Cannot execute SELL order." << std::endl;
            return;
        }
        for (const auto& level : book.bids) {
            double price = level.first;
            double available_quantity = level.second;
            double trade_quantity = std::min(quantity_to_fill, available_quantity);

            total_cost += trade_quantity * price;
            quantity_filled += trade_quantity;
            quantity_to_fill -= trade_quantity;

            if (quantity_to_fill < 1e-9) break;
        }
    }

    if (quantity_filled < order_event.quantity) {
        std::cout << "Execution Warning: Partial fill for " << order_event.symbol 
                  << ". Wanted: " << order_event.quantity << ", Filled: " << quantity_filled << std::endl;
    }
    
    if (quantity_filled <= 0) {
        std::cerr << "Execution Error: Could not fill any quantity for " << order_event.symbol << std::endl;
        return;
    }

    double fill_price = total_cost / quantity_filled;
    double commission = commission_->calculate(quantity_filled, fill_price);

    auto fill_event = std::make_shared<FillEvent>(
        order_event.timestamp,
        order_event.symbol,
        order_event.strategy_name,
        order_event.direction,
        quantity_filled, // Use the actual filled quantity
        fill_price,
        commission
    );

    event_queue_->push(std::make_shared<std::shared_ptr<Event>>(fill_event));
}