#include "HistoricCSVDataHandler.h"
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

// Constructor: Initializes the handler by parsing the CSV file.
HistoricCSVDataHandler::HistoricCSVDataHandler(const string& csv_filepath, const string& symbol)
    : symbol(symbol) {
    
    // Open the file stream
    ifstream file(csv_filepath);
    if (!file.is_open()) {
        throw runtime_error("Could not open CSV file: " + csv_filepath);
    }

    string line;
    // Read and discard the header line
    if (!getline(file, line)) {
        throw runtime_error("Cannot read header from CSV file.");
    }

    // Read the file line by line
    while (getline(file, line)) {
        stringstream ss(line);
        string item;
        Bar bar;

        // Use a temporary string for the timestamp to convert to uint64_t
        string timestamp_str;
        getline(ss, timestamp_str, ',');
        bar.timestamp = stoull(timestamp_str);

        // Read open, high, low, close, volume
        getline(ss, item, ',');
        bar.open = stod(item);
        getline(ss, item, ',');
        bar.high = stod(item);
        getline(ss, item, ',');
        bar.low = stod(item);
        getline(ss, item, ',');
        bar.close = stod(item);
        getline(ss, item, ',');
        bar.volume = stod(item);
        
        // Add the parsed bar to our vector
        this->bars.push_back(bar);
    }

    // Initialize the iterator to the beginning of the bars vector
    this->bar_iterator = this->bars.cbegin();
}

// Returns the next bar from the loaded data.
optional<Bar> HistoricCSVDataHandler::getLatestBar() {
    // Check if the iterator has reached the end of the vector
    if (bar_iterator != bars.cend()) {
        // Get the current bar
        Bar bar = *bar_iterator;
        // Advance the iterator to the next bar
        ++bar_iterator;
        // Return the bar wrapped in an optional
        return bar;
    } else {
        // If we are at the end, return an empty optional
        return nullopt;
    }
}
