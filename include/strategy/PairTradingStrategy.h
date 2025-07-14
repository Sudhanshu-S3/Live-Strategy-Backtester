#ifndef PAIR_TRADING_STRATEGY_H
#define PAIR_TRADING_STRATEGY_H

#include <string>
#include <vector>
#include <map>
#include "Strategy.h"
#include "DataHandler.h" // Include DataHandler
#include <memory>      // For std::shared_ptr

class PairTradingStrategy : public Strategy {
public:
    PairTradingStrategy(
        std::string symbol_a, 
        std::string symbol_b, 
        int window, 
        double z_score_threshold,
        std::shared_ptr<DataHandler> data_handler, // Add data_handler
        std::shared_ptr<std::queue<std::shared_ptr<Event>>> events_queue
    );

    void onMarket(
        const MarketEvent& event
    ) override;

private:
    // --- Parameters ---
    std::string symbol_a;
    std::string symbol_b;
    int window;
    double z_score_threshold;
    std::shared_ptr<DataHandler> data_handler; // Add data_handler member

    // --- State ---
    // Enum to track if we are long the pair (long A, short B), short the pair, or flat.
    enum class PositionState { NONE, LONG, SHORT };
    PositionState current_position = PositionState::NONE;

    // Internal storage for the latest prices and the history of the price ratio.
    std::map<std::string, double> latest_prices;
    std::vector<double> ratio_history;
};

#endif // PAIR_TRADING_STRATEGY_H