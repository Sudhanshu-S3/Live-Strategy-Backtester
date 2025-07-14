#ifndef SIMPLE_MOVING_AVERAGE_CROSSOVER_H
#define SIMPLE_MOVING_AVERAGE_CROSSOVER_H

#include "Strategy.h"
#include <vector>
#include <string>
using namespace std;

class SimpleMovingAverageCrossover : public Strategy {
public:
    // Constructor takes the symbol and the short/long window periods.
    SimpleMovingAverageCrossover(string symbol, int short_window, int long_window);

    // Override the virtual function from the base class.
    SignalEvent generateSignals(const Bar& bar) override;

private:
    string symbol;
    int short_window;
    int long_window;
    vector<double> prices; 

    // Simple Moving Average 
    double calculate_sma(int period);
};

#endif