#include "../../include/data/HistoricCSVDataHandler.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip> // Required for get_time

using namespace std;

HistoricCSVDataHandler::HistoricCSVDataHandler(const map<string, string>& csv_filepaths) {
    parse_all_csvs(csv_filepaths);

    // Initialize the iterators to the beginning of each symbol's bar vector
    for (const auto& pair : all_bars) {
        current_bar_iterators[pair.first] = pair.second.cbegin();
    }
}

// NOTE: This parser assumes a CSV format of:
// Timestamp,Open,High,Low,Close,Volume
// e.g., 2023-01-01 00:00:00,30000.1,30005.2,29990.8,30002.5,150.7
void HistoricCSVDataHandler::parse_all_csvs(const map<string, string>& csv_filepaths) {
    for (const auto& pair : csv_filepaths) {
        const string& symbol = pair.first;
        const string& filepath = pair.second;
        
        ifstream file(filepath);
        if (!file.is_open()) {
            throw runtime_error("Could not open CSV file: " + filepath);
        }

        string line;
        // Skip header
        getline(file, line);

        vector<Bar> bars_for_symbol;
        while (getline(file, line)) {
            stringstream ss(line);
            string item;
            
            Bar bar;
            bar.symbol = symbol;

            // 1. Timestamp
            getline(ss, bar.timestamp, ',');

            // 2. Open, High, Low, Close, Volume
            getline(ss, item, ','); bar.open = stod(item);
            getline(ss, item, ','); bar.high = stod(item);
            getline(ss, item, ','); bar.low = stod(item);
            getline(ss, item, ','); bar.close = stod(item);
            getline(ss, item, ','); bar.volume = stoll(item);

            bars_for_symbol.push_back(bar);
        }
        all_bars[symbol] = bars_for_symbol;
        cout << "Loaded " << bars_for_symbol.size() << " bars for symbol " << symbol << endl;
    }
}


// --- Interface Implementations ---

bool HistoricCSVDataHandler::isFinished() const {
    // The backtest is finished if all bar iterators have reached the end.
    for (const auto& pair : current_bar_iterators) {
        const auto& symbol = pair.first;
        if (pair.second != all_bars.at(symbol).cend()) {
            return false; // Found at least one stream that is not finished
        }
    }
    return true;
}

optional<Bar> HistoricCSVDataHandler::getLatestBar(const string& symbol) const {
    auto it = latest_bars_map.find(symbol);
    if (it != latest_bars_map.end()) {
        return it->second;
    }
    return nullopt;
}

void HistoricCSVDataHandler::updateBars(queue<shared_ptr<Event>>& event_queue) {
    if (isFinished()) {
        return; // Do nothing if all data streams are exhausted
    }

    // Determine which symbol has the next bar in chronological order
    string next_symbol_to_process = "";
    string earliest_time = "9999-99-99"; // Use string comparison

    for (const auto& pair : current_bar_iterators) {
        const auto& symbol = pair.first;
        const auto& iterator = pair.second;

        // Check if this symbol's data stream is not exhausted
        if (iterator != all_bars.at(symbol).cend()) {
            // If this bar's time is earlier than the earliest found so far
            if (iterator->timestamp < earliest_time) {
                earliest_time = iterator->timestamp;
                next_symbol_to_process = symbol;
            }
        }
    }

    // If we found a next bar, create a MarketEvent and advance the corresponding iterator
    if (!next_symbol_to_process.empty()) {
        // Create and push the event
        const auto& bar_to_process = *current_bar_iterators.at(next_symbol_to_process);
        auto market_event = make_shared<MarketEvent>(bar_to_process.symbol, bar_to_process.timestamp);
        event_queue.push(market_event);

        // Update the latest bar for this symbol for easy access
        latest_bars_map[next_symbol_to_process] = bar_to_process;

        // Advance the iterator for ONLY the processed symbol
        current_bar_iterators.at(next_symbol_to_process)++;
    }
}
