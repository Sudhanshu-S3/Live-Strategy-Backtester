#include <iostream>
#include <memory>
#include <stdexcept>
#include "HistoricCSVDataHandler.h"
#include "DataTypes.h"

using namespace std;

string signalTypeToString(SignalType type) {
    switch (type) {
        case SignalType::LONG: return "LONG";
        case SignalType::SHORT: return "SHORT";
        case SignalType::EXIT: return "EXIT";
        case SignalType::DO_NOTHING: return "DO_NOTHING";
        default: return "UNKNOWN";
    }
}

int main() {
    cout << "Testing HistoricCSVDataHandler..." << endl;

    // Define the path to your data file.
    // Ensure the 'data' directory is in your project root.
    const string csv_filepath = "../data/BTCUSDT-1h-2025-07-13.csv";
    const string symbol = "BTCUSDT";

    try {
        // Create the data handler
        auto data_handler = make_unique<HistoricCSVDataHandler>(csv_filepath, symbol);

        cout << "\n--- Reading Bars from CSV ---" << endl;

        // Loop to get all bars from the handler
        while (auto bar_optional = data_handler->getLatestBar()) {
            // The optional contains a value, so we can access it
            const Bar& bar = *bar_optional;
            
            cout << "Timestamp: " << bar.timestamp
                    << ", Open: " << bar.open
                    << ", High: " << bar.high
                    << ", Low: " << bar.low
                    << ", Close: " << bar.close
                    << ", Volume: " << bar.volume << endl;
        }

        cout << "\n--- End of Data ---" << endl;

    } catch (const runtime_error& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    cout << "\nTest complete. Data handler is working!" << endl;

    return 0;
}