#include "Backtester.h"
#include <iostream>
using namespace std;

Backtester::Backtester(unique_ptr<DataHandler> data_handler, unique_ptr<Strategy> strategy)
    : data_handler(move(data_handler)), strategy(move(strategy)) {}

// Convert SignalType to a string for printing.
string signalTypeToStringUtil(SignalType type) {
    switch (type) {
        case SignalType::LONG: return "LONG";
        case SignalType::SHORT: return "SHORT";
        case SignalType::EXIT: return "EXIT";
        case SignalType::DO_NOTHING: return "DO_NOTHING";
        default: return "UNKNOWN";
    }
}

// Main event loop.
void Backtester::run() {
    int bar_count = 0;
    while (auto bar_optional = data_handler->getLatestBar()) {
        bar_count++;
        const Bar& bar = *bar_optional;

        // Pass the bar to the strategy to get a signal
        SignalEvent signal = strategy->generateSignals(bar);

        cout << "Bar #" << bar_count << " | Timestamp: " << bar.timestamp << ", Close: " << bar.close;

        if (signal.type != SignalType::DO_NOTHING) {
            cout << " ==> SIGNAL: " << signalTypeToStringUtil(signal.type);
        }
        cout << endl;
    }
}