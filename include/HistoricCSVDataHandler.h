#ifndef HISTORIC_CSV_DATA_HANDLER_H
#define HISTORIC_CSV_DATA_HANDLER_H

#include <string>
#include <vector>
#include "DataHandler.h"
#include "DataTypes.h"

using namespace std;

// Reads OHLCV data from a CSV file and provides it bar by bar.
class HistoricCSVDataHandler : public DataHandler {
public:
    // Constructor takes the path to the CSV file and the symbol name.
    HistoricCSVDataHandler(const string& csv_filepath, const string& symbol);

    // Override the virtual function from the base class.
    optional<Bar> getLatestBar() override;

private:
    string symbol;
    vector<Bar> bars;
    vector<Bar>::const_iterator bar_iterator;

    void parse_csv();
};

#endif 