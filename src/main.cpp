#include <iostream>
#include <memory>
#include <iomanip>
#include <fstream>
#include <stdexcept>

// Include the JSON library
#include "nlohmann/json.hpp"
using namespace std;

// Include all our project components
#include "HistoricCSVDataHandler.h"
#include "SimpleMovingAverageCrossover.h"
#include "SimulatedExecutionHandler.h"
#include "Portfolio.h"
#include "Performance.h"
#include "Backtester.h"

// Use the nlohmann::json namespace
using json = nlohmann::json;

int main() {
    cout << "Starting Configuration-Driven Backtest..." << endl;

    // Load Configuration from JSON file
    json config;
    try {
        ifstream config_file("config.json");
        if (!config_file.is_open()) {
            throw runtime_error("Could not open config.json");
        }
        config = json::parse(config_file);
    } catch (const exception& e) {
        cerr << "Configuration Error: " << e.what() << endl;
        return 1;
    }

    // Instantiate Components Based on Configuration
    double initial_capital = config["initial_capital"];
    
    // We need a raw pointer to the portfolio to access its state after the run.
    Portfolio* portfolio_ptr = nullptr;

    try {
        // Create components using values from the config file
        auto data_handler = make_unique<HistoricCSVDataHandler>(
            config["data"]["csv_filepath"], 
            config["data"]["symbol"]
        );

        auto strategy = make_unique<SimpleMovingAverageCrossover>(
            config["data"]["symbol"],
            config["strategy"]["short_window"],
            config["strategy"]["long_window"]
        );

        auto execution_handler = make_unique<SimulatedExecutionHandler>(
            config["execution"]["commission_rate"]
        );

        auto portfolio = make_unique<Portfolio>(initial_capital);
        portfolio_ptr = portfolio.get();

        // Inject Dependencies into the Backtester
        // The Backtester is constructed with the abstract base classes,
        // completely unaware of the concrete implementations.
        auto backtester = make_unique<Backtester>(
            move(data_handler), 
            move(strategy), 
            move(execution_handler),
            portfolio_ptr
        );

        // Run the backtest 
        backtester->run();

    } catch (const exception& e) {
        cerr << "Runtime Error: " << e.what() << endl;
        return 1;
    }

    // Generate and Display Final Performance Report
    cout << "\n<><><><><><><><><><><><><><><><><><><><><><><><><>" << endl;
    cout << "           STRATEGY PERFORMANCE REPORT" << endl;
    cout << "<><><><><><><><><><><><><><><><><><><><><><><><><>" << endl;

    if (portfolio_ptr) {
        Performance perf(portfolio_ptr->getEquityCurve(), initial_capital);
        cout << fixed << setprecision(2);

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
