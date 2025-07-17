#include "strategy/StrategyFactory.h"
#include "strategy/OrderBookImbalanceStrategy.h"
#include <stdexcept>

std::shared_ptr<Strategy> StrategyFactory::createStrategy(
    const nlohmann::json& strategy_config,
    std::shared_ptr<EventQueue> event_queue,
    std::shared_ptr<DataHandler> data_handler)
{
    std::string strategy_name = strategy_config["name"];
    
    if (strategy_name == "ORDER_BOOK_IMBALANCE") {
        auto params = strategy_config["params"];
        return std::make_shared<OrderBookImbalanceStrategy>(
            event_queue, 
            data_handler, 
            strategy_config["symbol"],
            params["lookback_levels"], 
            params["imbalance_threshold"]
        );
    }
    // Add other strategies here with else if
    
    throw std::runtime_error("Strategy not recognized: " + strategy_name);
}
