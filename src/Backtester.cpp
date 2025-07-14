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
    unique_ptr<ExecutionHandler> execution_handler
) : data_handler(move(data_handler)), 
    strategy(move(strategy)),
    execution_handler(move(execution_handler)) {}


void Backtester::run() {
    int bar_count = 0;
    while (auto bar_optional = data_handler->getLatestBar()) {
        bar_count++;
        const Bar& bar = *bar_optional;

        // Pass the bar to the strategy to get a signal
        SignalEvent signal = strategy->generateSignals(bar);

        // If we get a real signal (not DO_NOTHING), process it.
        if (signal.type != SignalType::DO_NOTHING) {
            cout << "--------------------------------------------------------" << endl;
            cout << "Bar #" << bar_count << " | Timestamp: " << bar.timestamp << ", Close: " << bar.close << endl;
            cout << "  => SIGNAL: " << signalTypeToStringUtil(signal.type) << " on " << signal.symbol << endl;

            // Create an OrderEvent from the SignalEvent.
        
            OrderEvent order = {
                bar.timestamp,
                signal.symbol,
                (signal.type == SignalType::LONG) ? OrderDirection::BUY : OrderDirection::SELL,
                1.0 
            };
            cout << "  => CREATED ORDER: " << orderDirectionToStringUtil(order.direction)
                    << " " << order.quantity << " " << order.symbol << endl;

            
            FillEvent fill = execution_handler->executeOrder(order, bar);
            
            cout << "  => RECEIVED FILL: " << orderDirectionToStringUtil(fill.direction)
                    << " " << fill.quantity << " " << fill.symbol
                    << " at $" << fill.fill_price
                    << " | Commission: $" << fill.commission << endl;
            cout << "--------------------------------------------------------" << endl;
        }
    }
}
