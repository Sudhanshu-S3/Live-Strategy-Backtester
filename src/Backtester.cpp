#include "Backtester.h"
#include <iostream>

using namespace std;

inline string signalTypeToStringUtil(SignalType type) {
    switch (type) {
        case SignalType::LONG: return "LONG";
        case SignalType::SHORT: return "SHORT";
        case SignalType::EXIT: return "EXIT";
        case SignalType::DO_NOTHING: return "DO_NOTHING";
        default: return "UNKNOWN";
    }
}

inline string orderDirectionToStringUtil(OrderDirection dir) {
    switch (dir) {
        case OrderDirection::BUY: return "BUY";
        case OrderDirection::SELL: return "SELL";
        default: return "UNKNOWN";
    }
}

Backtester::Backtester(
    unique_ptr<DataHandler> data_handler, 
    unique_ptr<Strategy> strategy,
    unique_ptr<ExecutionHandler> execution_handler,
    Portfolio* portfolio // Change this to a raw pointer
) : data_handler(move(data_handler)), 
    strategy(move(strategy)),
    execution_handler(move(execution_handler)),
    portfolio(portfolio) {} // Assign the pointer


// The main event loop for the backtest.
void Backtester::run() {
    int bar_count = 0;
    while (auto bar_optional = data_handler->getLatestBar()) {
        bar_count++;
        // Correctly unpack the pair from the optional
        const pair<string, Bar>& symbol_bar_pair = *bar_optional;
        const string& symbol = symbol_bar_pair.first; // The symbol from the data handler
        const Bar& bar = symbol_bar_pair.second;           // The bar data

        
        portfolio->updateTimeIndex(symbol, bar);

        SignalEvent signal = strategy->generateSignals(bar);

    
        if (signal.type != SignalType::DO_NOTHING) {
            OrderEvent order;
            bool create_order = false;

            if (signal.type == SignalType::LONG) {
                
                if (portfolio->isHolding(signal.symbol) && !portfolio->isLong(signal.symbol)) {
                    OrderEvent exit_order = {bar.timestamp, signal.symbol, OrderDirection::BUY, 1.0};
                    FillEvent exit_fill = execution_handler->executeOrder(exit_order, bar);
                    portfolio->onFill(exit_fill);
                }
                
                if (!portfolio->isHolding(signal.symbol)) {
                    order = {bar.timestamp, signal.symbol, OrderDirection::BUY, 1.0};
                    create_order = true;
                }
            } else if (signal.type == SignalType::SHORT) {
                
                if (portfolio->isHolding(signal.symbol) && portfolio->isLong(signal.symbol)) {
                    OrderEvent exit_order = {bar.timestamp, signal.symbol, OrderDirection::SELL, 1.0};
                    FillEvent exit_fill = execution_handler->executeOrder(exit_order, bar);
                    portfolio->onFill(exit_fill);
                }
                
                if (!portfolio->isHolding(signal.symbol)) {
                    order = {bar.timestamp, signal.symbol, OrderDirection::SELL, 1.0};
                    create_order = true;
                }
            } else if (signal.type == SignalType::EXIT) {
                // Only exit if we are in a position
                if (portfolio->isHolding(signal.symbol)) {
                    OrderDirection direction = portfolio->isLong(signal.symbol) ? OrderDirection::SELL : OrderDirection::BUY;
                    order = {bar.timestamp, signal.symbol, direction, 1.0};
                    create_order = true;
                }
            }

            if (create_order) {
                FillEvent fill = execution_handler->executeOrder(order, bar);
                portfolio->onFill(fill);

                cout << "--------------------------------------------------------" << endl;
                cout << "Bar #" << bar_count << " | Timestamp: " << bar.timestamp << ", Close: " << bar.close << endl;
                cout << "  => EXECUTED TRADE: " << orderDirectionToStringUtil(fill.direction)
                        << " " << fill.quantity << " " << fill.symbol
                        << " at $" << fill.fill_price << endl;
                cout << "  => PORTFOLIO: Cash = " << portfolio->getCurrentCash() 
                        << ", Total Value = " << portfolio->getTotalValue() << endl;
                cout << "--------------------------------------------------------" << endl;
            }
        }
    }
}