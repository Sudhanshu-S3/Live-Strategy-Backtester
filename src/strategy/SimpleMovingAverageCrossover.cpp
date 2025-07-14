#include "SimpleMovingAverageCrossover.h"
#include <numeric> 
using namespace std;

SimpleMovingAverageCrossover::SimpleMovingAverageCrossover(string symbol, int short_window, int long_window)
    : symbol(symbol), short_window(short_window), long_window(long_window) {}


double SimpleMovingAverageCrossover::calculate_sma(int period) {
    
    if (prices.size() < period) {
        return 0.0;
    }

    double sum = accumulate(prices.end() - period, prices.end(), 0.0);
    return sum / period;
}


SignalEvent SimpleMovingAverageCrossover::generateSignals(const Bar& bar) {
    
    prices.push_back(bar.close);

    // 'long_window' + 1 bars to have two full SMAs to compare
    if (prices.size() < long_window + 1) {
        return { bar.timestamp, this->symbol, SignalType::DO_NOTHING };
    }

    // Calculate the short and long SMAs for the current and previous bar
    double short_sma_now = calculate_sma(short_window);
    double long_sma_now = calculate_sma(long_window);

    // To detect a crossover, we need the previous state.

    vector<double> prev_prices(prices.begin(), prices.end() - 1);
    double prev_short_sma = 0.0;
    double prev_long_sma = 0.0;
    if (prev_prices.size() >= long_window) {
        double short_sum = accumulate(prev_prices.end() - short_window, prev_prices.end(), 0.0);
        prev_short_sma = short_sum / short_window;

        double long_sum = accumulate(prev_prices.end() - long_window, prev_prices.end(), 0.0);
        prev_long_sma = long_sum / long_window;
    }

    // Check for crossover conditions
    
    if (prev_short_sma < prev_long_sma && short_sma_now > long_sma_now) {
        return { bar.timestamp, this->symbol, SignalType::LONG };
    }
    // Death Cross (SHORT signal): Short SMA was above Long SMA, and now it's below.
    else if (prev_short_sma > prev_long_sma && short_sma_now < long_sma_now) {
        return { bar.timestamp, this->symbol, SignalType::SHORT };
    }

    return { bar.timestamp, this->symbol, SignalType::DO_NOTHING };
}
