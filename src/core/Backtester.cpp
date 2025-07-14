#include "../../include/core/Backtester.h"
#include <iostream>

// Constructor initializes all the shared pointers and the backtest flag.
Backtester::Backtester(
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
    std::shared_ptr<DataHandler> data_handler,
    std::shared_ptr<Strategy> strategy,
    std::shared_ptr<Portfolio> portfolio,
    std::shared_ptr<ExecutionHandler> execution_handler
) : event_queue(event_queue),
    data_handler(data_handler),
    strategy(strategy),
    portfolio(portfolio),
    execution_handler(execution_handler),
    continue_backtest(true) {}


// The main event loop for the backtest.
void Backtester::run() {
    while (continue_backtest) {
        // 1. Ask the Data Handler to generate new MarketEvents from the data source.
        data_handler->updateBars(*event_queue);

        // If the data handler has no more data, we'll stop after this loop.
        if (data_handler->isFinished()) {
            continue_backtest = false;
        }

        // 2. Process all events currently in the queue.
        while (!event_queue->empty()) {
            std::shared_ptr<Event> event = event_queue->front();
            event_queue->pop();

            // Dispatch the event to the appropriate handler method.
            handleEvent(event);
        }
    }
    std::cout << "Backtest event loop finished." << std::endl;
}


// Routes events to the correct component based on their type.
void Backtester::handleEvent(const std::shared_ptr<Event>& event) {
    if (auto market_event = std::dynamic_pointer_cast<MarketEvent>(event)) {
        // A new bar of data has arrived.
        // First, update the portfolio's value with the latest prices.
        portfolio->updateTimeIndex();
        // Then, let the strategy react to the new market data.
        strategy->generateSignals(*market_event, *event_queue);
    }
    else if (auto signal_event = std::dynamic_pointer_cast<SignalEvent>(event)) {
        // The strategy has signaled a desire to trade.
        // We convert this signal into a concrete OrderEvent.
        // NOTE: In a more advanced system, a RiskManager would sit here to size the order.
        double quantity = 100.0; // Example: trade a fixed quantity of 100 units
        std::string order_type = "MKT"; // Market order
        std::string direction;

        if (signal_event->signal_type == "LONG") {
            direction = "BUY";
        } else if (signal_event->signal_type == "SHORT") {
            direction = "SELL";
        } else if (signal_event->signal_type == "EXIT") {
            // To exit, we must check the portfolio to know our current position.
            direction = portfolio->getPositionDirection(signal_event->symbol);
            if (direction == "NONE") return; // No position to exit, do nothing.
        } else {
            return; // Unknown signal type
        }

        auto order = std::make_shared<OrderEvent>(
            signal_event->symbol,
            signal_event->timestamp,
            order_type,
            direction,
            quantity
        );
        event_queue->push(order); // Push the new OrderEvent onto the queue.
    }
    else if (auto order_event = std::dynamic_pointer_cast<OrderEvent>(event)) {
        // An order needs to be executed.
        // We need the latest bar for the symbol to simulate the execution price.
        auto bar = data_handler->getLatestBar(order_event->symbol);
        if (bar) {
            execution_handler->executeOrder(*order_event, *bar);
        } else {
            std::cerr << "Could not get latest bar for symbol " << order_event->symbol << " to execute order."
                      << std::endl;
        }
    }
    else if (auto fill_event = std::dynamic_pointer_cast<FillEvent>(event)) {
        // The order was filled. Update the portfolio with the trade details.
        portfolio->onFill(*fill_event);
    }
}