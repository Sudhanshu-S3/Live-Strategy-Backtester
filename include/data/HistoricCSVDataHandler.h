#ifndef HISTORIC_CSV_DATA_HANDLER_H
#define HISTORIC_CSV_DATA_HANDLER_H

#include "DataHandler.h"
#include "DataTypes.h"
#include "event/Event.h"
#include "../../include/event/EventQueue.h" // Add this line to include EventQueue definition
#include "../../../lib/mio/mio.hpp"
#include "../event/ThreadSafeQueue.h"
#include <optional> // For std::optional
#include <string>
#include <unordered_map>
#include <vector>

// HistoricCSVDataHandler loads data for multiple symbols from CSV files
// and feeds them to the backtester in chronological order.
class HistoricCSVDataHandler : public DataHandler {
public:
    // Constructor now takes a map of {symbol -> filepath}.
    HistoricCSVDataHandler(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue, const std::map<std::string, std::string>& csv_filepaths);
    // Overload for single file (e.g., benchmark)
    HistoricCSVDataHandler(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue, const std::string& symbol, const std::string& filepath);
    HistoricCSVDataHandler(EventQueue& events, std::string csv_dir, std::vector<std::string> symbols);
    ~HistoricCSVDataHandler() override = default;

    // Override the new interface methods.
    void updateBars() override;
    void continue_backtest();
    bool isFinished() const override;
    std::optional<Bar> getLatestBar(const std::string& symbol) const override;
    double getLatestBarValue(const std::string& symbol, const std::string& val_type) override;
    std::vector<Bar> getLatestBars(const std::string& symbol, int n = 1) override;

private:
    void open_and_map_csv(const std::string& symbol);

    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue_;
    std::string csv_dir_;
    std::vector<std::string> symbols_;
    std::unordered_map<std::string, mio::mmap_source> mapped_files_;
    std::unordered_map<std::string, const char*> file_cursors_;

    // Stores all bars for all symbols, loaded at the start.
    // Key: symbol (e.g., "BTC/USDT"), Value: Vector of Bars for that symbol.
    std::map<std::string, std::vector<Bar>> all_bars;

    // Keeps track of the current position (iterator) in each symbol's vector of bars.
    std::map<std::string, std::vector<Bar>::const_iterator> current_bar_iterators;

    // A helper to store the most recently processed bar for each symbol for quick access.
    mutable std::map<std::string, Bar> latest_bars_map;

    // Called by the constructor to load and parse all specified CSV files.
    void parse_all_csvs(const std::map<std::string, std::string>& csv_filepaths);
    void parse_single_csv(const std::string& symbol, const std::string& filepath);
};

#endif // HISTORIC_CSV_DATA_HANDLER_H