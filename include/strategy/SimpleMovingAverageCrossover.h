#ifndef SIMPLE_MOVING_AVERAGE_CROSSOVER_H
#define SIMPLE_MOVING_AVERAGE_CROSSOVER_H

#include "Strategy.h"
#include "../data/DataHandler.h"
#include <string>
#include <vector>
#include <memory>

class SimpleMovingAverageCrossover : public Strategy {
public:
    SimpleMovingAverageCrossover(
        std::string symbol, 
        int short_window, 
        int long_window,
        std::shared_ptr<DataHandler> data_handler
    );

    void generateSignals(
        const MarketEvent& event, 
        std::queue<std::shared_ptr<Event>>& event_queue
    ) override;

private:
    double calculate_sma(int period);

    std::string symbol;
    int short_window;
    int long_window;
    std::shared_ptr<DataHandler> data_handler;

    std::vector<double> prices;
    enum class PositionState { NONE, LONG, SHORT };
    PositionState current_position = PositionState::NONE;
};

#endif // SIMPLE_MOVING_AVERAGE_CROSSOVER_H