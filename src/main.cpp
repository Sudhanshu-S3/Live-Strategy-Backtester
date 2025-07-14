#include <iostream>
#include <memory>
#include "HistoricCSVDataHandler.h"
#include "SimpleMovingAverageCrossover.h"
#include "SimulatedExecutionHandler.h" 
#include "Backtester.h"
using namespace std;


int main() {
    cout << "Starting Backtest with Execution Simulation..." << endl;

    // Use the new data file provided for Day 2
    const string csv_filepath = "../data/BTCUSDT-1h-2025-07-13.csv";
    const string symbol = "BTCUSDT";
    const int short_window = 10;
    const int long_window = 25;

    try {
        // Create the Data Handler
        auto data_handler = make_unique<HistoricCSVDataHandler>(csv_filepath, symbol);

        // Create the Strategy
        auto strategy = make_unique<SimpleMovingAverageCrossover>(symbol, short_window, long_window);

        // Create the Execution Handler
        auto execution_handler = make_unique<SimulatedExecutionHandler>();

        // Create the Backtester and pass it all the components
        auto backtester = make_unique<Backtester>(
            move(data_handler), 
            move(strategy), 
            move(execution_handler)
        );

        // Run the backtest!
        backtester->run();

    } catch (const runtime_error& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    cout << "\nBacktest complete!" << endl;

    return 0;
}
