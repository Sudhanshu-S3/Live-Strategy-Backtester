#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "DataTypes.h" 

using namespace std;


// Convert SignalType to a string for printing
string signalTypeToString(SignalType type) {
    switch (type) {
        case SignalType::LONG: return "LONG";
        case SignalType::SHORT: return "SHORT";
        case SignalType::EXIT: return "EXIT";
        case SignalType::DO_NOTHING: return "DO_NOTHING";
        default: return "UNKNOWN";
    }
}

// Function to read CSV data and convert it to Bar objects
vector<Bar> readCsvFile(const string& filename) {
    vector<Bar> bars;
    ifstream file(filename);
    
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        return bars;
    }
    
    string line;
    while (getline(file, line)) {
        // Skip comment lines or empty lines
        if (line.empty() || line.substr(0, 2) == "//") {
            continue;
        }
        
        stringstream ss(line);
        string token;
        Bar bar;
        
        // Parse timestamp
        if (getline(ss, token, ',')) {
            bar.timestamp = stoull(token) / 1000; // Convert microseconds to milliseconds
        }
        
        // Parse open price
        if (getline(ss, token, ',')) {
            bar.open = stod(token);
        }
        
        // Parse high price
        if (getline(ss, token, ',')) {
            bar.high = stod(token);
        }
        
        // Parse low price
        if (getline(ss, token, ',')) {
            bar.low = stod(token);
        }
        
        // Parse close price
        if (getline(ss, token, ',')) {
            bar.close = stod(token);
        }
        
        // Parse volume
        if (getline(ss, token, ',')) {
            bar.volume = stod(token);
        }
        
        bars.push_back(bar);
    }
    
    file.close();
    return bars;
}

int main() {
    cout << "Testing CSV Data Loading..." << endl;

    // Path to the CSV file
    string csvFilePath = "../data/BTCUSDT-1h-2025-07-13.csv";
    
    // Read the CSV file
    vector<Bar> bars = readCsvFile(csvFilePath);
    
    // Check if data was loaded successfully
    if (bars.empty()) {
        cerr << "No data was loaded from the CSV file." << endl;
        return 1;
    }
    
    cout << "Successfully loaded " << bars.size() << " bars from CSV." << endl;
    
    // Display the first 5 bars (or fewer if less are available)
    int barsToDisplay = min(5, static_cast<int>(bars.size()));
    for (int i = 0; i < barsToDisplay; i++) {
        cout << "\n--- Bar " << i+1 << " ---" << endl;
        cout << "Timestamp: " << bars[i].timestamp << endl;
        cout << "Open: " << bars[i].open << endl;
        cout << "High: " << bars[i].high << endl;
        cout << "Low: " << bars[i].low << endl;
        cout << "Close: " << bars[i].close << endl;
        cout << "Volume: " << bars[i].volume << endl;
    }

    // Create a signal based on the first bar's data
    if (!bars.empty()) {
        SignalEvent testSignal;
        testSignal.timestamp = bars[0].timestamp;
        testSignal.symbol = "BTCUSDT";
        testSignal.type = SignalType::LONG;

        cout << "\n--- Sample Signal Based on First Bar ---" << endl;
        cout << "Timestamp: " << testSignal.timestamp << endl;
        cout << "Symbol: " << testSignal.symbol << endl;
        cout << "Signal Type: " << signalTypeToString(testSignal.type) << endl;
    }

    cout << "\nCSV data loading test complete!" << endl;
    return 0;
}