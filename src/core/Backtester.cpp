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
// Updated to handle all event types, including new HFT events
void Backtester::handleEvent(const std::shared_ptr<Event>& event) {
    // Note: Using a switch statement is often more efficient than multiple dynamic_pointer_casts
    switch (event->type) {
        case Event::MARKET:
            // For now, these are disabled as we test the HFT pipeline
            // if (portfolio) portfolio->onMarket(static_cast<MarketEvent&>(*event));
            // if (strategy) strategy->onMarket(static_cast<MarketEvent&>(*event));
            break;
        
        case Event::TRADE: { // NEW CASE
            auto& trade_event = static_cast<TradeEvent&>(*event);
            // You can uncomment the line below to see the stream of trades
            // std::cout << "Trade | " << trade_event.trade.symbol << " | Price: " << trade_event.trade.price << std::endl;
            break;
        }

        case Event::ORDERBOOK: { // NEW CASE
            auto& book_event = static_cast<OrderBookEvent&>(*event);
            // Uncommenting this will produce a massive amount of output
            // std::cout << "Book | " << book_event.book.symbol << " | Top Bid: " << book_event.book.bids[0].price << std::endl;
            break;
        }

        case Event::SIGNAL:
            // if (portfolio) portfolio->onSignal(static_cast<SignalEvent&>(*event));
            break;

        case Event::ORDER:
            // if (execution_handler) execution_handler->onOrder(static_cast<OrderEvent&>(*event));
            break;

        case Event::FILL:
            // if (portfolio) portfolio->onFill(static_cast<FillEvent&>(*event));
            break;

        default:
            std::cout << "Warning: Unknown event type received in Backtester." << std::endl;
            break;
    }
}