#include "core/Backtester.h"
#include "data/DataTypes.h" // Include to recognize HFT event types
#include <iostream>

// --- Constructor ---
// (No changes needed to your constructor)
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
    execution_handler(execution_handler) {}


// --- Main Event Loop ---
// Updated to work with the new HFTDataHandler
void Backtester::run() {
    std::cout << "Backtester starting event loop..." << std::endl;
    int event_count = 0;

    // The loop continues as long as the data handler is not finished.
    while (!data_handler->isFinished()) {
        // 1. The DataHandler pushes events directly to the main event queue.
        data_handler->updateBars(*event_queue);

        // 2. Process all events currently in the queue.
        while (!event_queue->empty()) {
            std::shared_ptr<Event> event = event_queue->front();
            event_queue->pop();
            event_count++;

            // Log every 500,000 events to show progress without flooding the console
            if (event_count % 500000 == 0) {
                std::cout << "Processed " << event_count << " events..." << std::endl;
            }

            // Dispatch the event
            handleEvent(event);
        }
    }
    std::cout << "Backtester event loop finished. Total events processed: " << event_count << std::endl;
}

// --- Event Handling Logic ---
// Consolidated and corrected to handle all event types properly.
void Backtester::handleEvent(const std::shared_ptr<Event>& event) {
    // For high-frequency backtesting, update the portfolio's mark-to-market value on every event.
    if (portfolio) {
        portfolio->updateTimeIndex();
    }

    switch (event->type) {
        case Event::MARKET: {
            auto& market_event = static_cast<MarketEvent&>(*event);
            if (strategy) strategy->onMarket(market_event);
            if (portfolio) portfolio->onMarket(market_event);
            break;
        }
        case Event::TRADE: {
            auto& trade_event = static_cast<TradeEvent&>(*event);
            if (strategy) strategy->onTrade(trade_event);
            break;
        }
        case Event::ORDERBOOK: {
            auto& book_event = static_cast<OrderBookEvent&>(*event);
            if (strategy) strategy->onOrderBook(book_event);
            break;
        }
        case Event::SIGNAL: {
            auto& signal_event = static_cast<SignalEvent&>(*event);
            if (portfolio) portfolio->onSignal(signal_event);
            break;
        }
        case Event::ORDER: {
            auto& order_event = static_cast<OrderEvent&>(*event);
            if (execution_handler) execution_handler->onOrder(order_event);
            break;
        }
        case Event::FILL: {
            auto& fill_event = static_cast<FillEvent&>(*event);
            if (portfolio) portfolio->onFill(fill_event);
            if (strategy) strategy->onFill(fill_event); // Allow strategy to react to its own fills
            break;
        }
        default:
            std::cout << "Warning: Unknown event type received in Backtester." << std::endl;
            break;
    }
}