#include <iostream>
#include <memory>
#include <iomanip>
#include "HistoricCSVDataHandler.h"
#include "SimpleMovingAverageCrossover.h"
#include "SimulatedExecutionHandler.h"
#include "Portfolio.h"
#include "Performance.h"
#include "Backtester.h"

using namespace std;

int main() {
    cout << "Starting Full Backtest with Performance Analysis..." << endl;

    const string csv_filepath = "../data/BTCUSDT-1h-2025-07-13.csv";
    const string symbol = "BTCUSDT";
    const int short_window = 10;
    const int long_window = 25;
    const double initial_capital = 100000.0;

    Portfolio* portfolio_ptr = nullptr;

    try {
        auto portfolio = make_unique<Portfolio>(initial_capital);
        portfolio_ptr = portfolio.get();

        // Create all the other components
        auto data_handler = make_unique<HistoricCSVDataHandler>(csv_filepath, symbol);
        auto strategy = make_unique<SimpleMovingAverageCrossover>(symbol, short_window, long_window);
        auto execution_handler = make_unique<SimulatedExecutionHandler>(0.001);

        // Create the Backtester, moving ownership of components to it.
        auto backtester = make_unique<Backtester>(
            move(data_handler), 
            move(strategy), 
            move(execution_handler),
            portfolio_ptr // Pass the raw pointer
        );

        // Run the backtest!
        backtester->run();

    } catch (const runtime_error& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    // --- Generate and Display Final Performance Report ---
    cout << "\n<><><><><><><><><><><><><><><><><><><><><><><><><>" << endl;
    cout << "           STRATEGY PERFORMANCE REPORT" << endl;
    cout << "<><><><><><><><><><><><><><><><><><><><><><><><><>" << endl;

    if (portfolio_ptr) {
        // Create the performance calculator from the portfolio's results
        Performance perf(portfolio_ptr->getEquityCurve(), initial_capital);

        cout << fixed << setprecision(2); // Format output

        cout << "\n--- OVERVIEW ---" << endl;
        cout << "Initial Capital:       $" << initial_capital << endl;
        cout << "Final Portfolio Value: $" << portfolio_ptr->getTotalValue() << endl;

        cout << "\n--- KEY METRICS ---" << endl;
        cout << "Total Return:          " << perf.getTotalReturn() * 100.0 << "%" << endl;
        cout << "Max Drawdown:          " << perf.getMaxDrawdown() * 100.0 << "%" << endl;
        cout << "Annualized Sharpe Ratio: " << perf.getSharpeRatio() << endl;
    }
    
    cout << "\nBacktest complete!" << endl;

    return 0;
}
