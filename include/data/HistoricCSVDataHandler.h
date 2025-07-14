#ifndef HISTORIC_CSV_DATA_HANDLER_H
#define HISTORIC_CSV_DATA_HANDLER_H

#include <string>
#include <vector>
#include <map>
#include "DataHandler.h"
#include "DataTypes.h"

// HistoricCSVDataHandler loads data for multiple symbols from CSV files
// and feeds them to the backtester in chronological order.
class HistoricCSVDataHandler : public DataHandler {
public:
    // Constructor now takes a map of {symbol -> filepath}.
    HistoricCSVDataHandler(const std::map<std::string, std::string>& csv_filepaths);

    // Override the new interface methods.
    void updateBars(std::queue<std::shared_ptr<Event>>& event_queue) override;
    bool isFinished() const override;
    std::optional<Bar> getLatestBar(const std::string& symbol) const override;

private:
    // Stores all bars for all symbols, loaded at the start.
    // Key: symbol (e.g., "BTC/USDT"), Value: Vector of Bars for that symbol.
    std::map<std::string, std::vector<Bar>> all_bars;

    // Keeps track of the current position (iterator) in each symbol's vector of bars.
    std::map<std::string, std::vector<Bar>::const_iterator> current_bar_iterators;

    // A helper to store the most recently processed bar for each symbol for quick access.
    mutable std::map<std::string, Bar> latest_bars_map;

    // Called by the constructor to load and parse all specified CSV files.
    void parse_all_csvs(const std::map<std::string, std::string>& csv_filepaths);
};

#endif