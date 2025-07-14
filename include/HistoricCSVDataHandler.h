#ifndef HISTORIC_CSV_DATA_HANDLER_H
#define HISTORIC_CSV_DATA_HANDLER_H

#include <string>
#include <vector>
#include "DataHandler.h"
#include "DataTypes.h"

using namespace std;

class HistoricCSVDataHandler : public DataHandler {
public:
    
    HistoricCSVDataHandler(const string& csv_filepath, const string& symbol);

    optional<pair<string, Bar>> getLatestBar() override;

private:
    string symbol;
    vector<Bar> bars;
    vector<Bar>::const_iterator bar_iterator;

    void parse_csv();
};

#endif