#ifndef HFT_DATA_HANDLER_H
#define HFT_DATA_HANDLER_H

#include "data/DataHandler.h"
#include "data/DataTypes.h"
#include <fstream>
#include <map>
#include <vector>
#include <queue>
#include <memory>

// HFTDataHandler is designed to handle high-frequency trade and order book
// data. It reads from CSV files, combines the data streams, and pushes
// events (TradeEvent, OrderBookEvent) to the central queue in chronological order.
class HFTDataHandler : public DataHandler {
public:
    // Constructor: Takes a list of symbols and the path to the data files.
    HFTDataHandler(const std::vector<std::string>& symbols,
                   const std::string& trade_data_dir,
                   const std::string& book_data_dir);

    virtual ~HFTDataHandler() = default;

    // The core update function. It determines which event (trade or book) is next
    // across all symbols and pushes it to the event queue.
    void updateBars(std::queue<std::shared_ptr<Event>>& event_queue) override;

    // Checks if all data streams have been processed.
    bool isFinished() const override;

    // HFT data is event-driven (trades/books), not bar-based.
    // This function is not applicable and returns no data.
    std::optional<Bar> getLatestBar(const std::string& symbol) const override;
    
    // Get the last known value for a symbol (e.g., price). Overrides
    double getLatestBarValue(const std::string& symbol, const std::string& val_type) override;

    // This function is not applicable and returns no data.
    std::vector<Bar> getLatestBars(const std::string& symbol, int n = 1) override; 


private:
    // Helper function to load a trade CSV file for a given symbol.
    bool load_trade_data(const std::string& symbol);

    // Helper function to load an order book CSV file for a given symbol.
    bool load_orderbook_data(const std::string& symbol);

    std::vector<std::string> symbols_; // List of symbols we are tracking.

    // The directory paths for our historical data.
    std::string trade_data_dir_;
    std::string book_data_dir_;

    // In-memory storage for all trade and order book data, mapped by symbol.
    std::map<std::string, std::vector<Trade>> all_trades_;
    std::map<std::string, std::vector<OrderBook>> all_orderbooks_;

    // Pointers (indices) to the current position in each data vector for streaming.
    std::map<std::string, size_t> trade_indices_;
    std::map<std::string, size_t> orderbook_indices_;
    std::map<std::string, double> latest_prices_;
};

#endif // HFT_DATA_HANDLER_H