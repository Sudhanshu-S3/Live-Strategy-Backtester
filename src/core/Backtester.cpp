#include "../../include/core/Backtester.h"
#include "data/DataTypes.h"
#include <iostream>
#include "../../include/risk/RishManager.h"

Backtester::Backtester(
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
    std::shared_ptr<DataHandler> data_handler,
    std::shared_ptr<Strategy> strategy,
    std::shared_ptr<Portfolio> portfolio,
    std::shared_ptr<ExecutionHandler> execution_handler,
    std::shared_ptr<RiskManager> risk_manager
) : event_queue(event_queue),
    data_handler(data_handler),
    strategy(strategy),
    portfolio(portfolio),
    execution_handler(execution_handler),
    risk_manager(risk_manager),
    continue_backtest(true) {}

void Backtester::run() {
    std::cout << "Backtester starting event loop..." << std::endl;
    int event_count = 0;

    while (continue_backtest && !data_handler->isFinished()) {
        data_handler->updateBars(*event_queue);

        while (!event_queue->empty()) {
            std::shared_ptr<Event> event = event_queue->front();
            event_queue->pop();
            event_count++;
            
            if (event_count % 500000 == 0) {
                std::cout << "Processed " << event_count << " events..." << std::endl;
            }
            
            handleEvent(event);
        }
    }
    std::cout << "Backtester event loop finished. Total events processed: " << event_count << std::endl;
}

void Backtester::handleEvent(const std::shared_ptr<Event>& event) {
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
            // The RiskManager now handles signals
            if (risk_manager) risk_manager->onSignal(signal_event);
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
            if (strategy) strategy->onFill(fill_event); 
            break;
        }
        default:
            std::cout << "Warning: Unknown event type received in Backtester." << std::endl;
            break;
    }
}