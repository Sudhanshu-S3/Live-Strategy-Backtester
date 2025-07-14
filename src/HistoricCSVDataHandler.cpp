#include "HistoricCSVDataHandler.h"
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;


HistoricCSVDataHandler::HistoricCSVDataHandler(const string& csv_filepath, const string& symbol)
    : symbol(symbol) {
    

    ifstream file(csv_filepath);
    if (!file.is_open()) {
        throw runtime_error("Could not open CSV file: " + csv_filepath);
    }

    string line;

    if (!getline(file, line)) {
        throw runtime_error("Cannot read header from CSV file.");
    }

    
    while (getline(file, line)) {
        stringstream ss(line);
        string item;
        Bar bar;

        string timestamp_str;
        getline(ss, timestamp_str, ',');
        bar.timestamp = stoull(timestamp_str);

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
        
    
        this->bars.push_back(bar);
    }

    
    this->bar_iterator = this->bars.cbegin();
}


optional<pair<string, Bar>> HistoricCSVDataHandler::getLatestBar() {
    if (bar_iterator != bars.end()) {
        const Bar& bar = *bar_iterator;
        bar_iterator++;
        return {{this->symbol, bar}};
    }
    return nullopt; 
}

void HistoricCSVDataHandler::parse_csv() {
    
}
