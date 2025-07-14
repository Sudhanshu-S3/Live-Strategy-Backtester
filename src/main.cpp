#include <iostream>
#include <memory>
#include "HistoricCSVDataHandler.h"
#include "SimpleMovingAverageCrossover.h"
#include "Backtester.h"

using namespace std;

int main() {
    cout << "Starting SMA Crossover Backtest..." << endl;
    const string csv_filepath = "../data/BTCUSDT-1h-2025-07-13.csv";
    const string symbol = "BTCUSDT";
    const int short_window = 10;
    const int long_window = 25;

    try {
        // Create the Data Handler
        auto data_handler = make_unique<HistoricCSVDataHandler>(csv_filepath, symbol);

        // Create the Strategy
        auto strategy = make_unique<SimpleMovingAverageCrossover>(symbol, short_window, long_window);

        // Create the Backtester and pass it the data and strategy
        auto backtester = make_unique<Backtester>(move(data_handler), move(strategy));

        // Run the backtest!
        backtester->run();

    } catch (const runtime_error& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    cout << "\nBacktest complete!" << endl;

    return 0;
}
