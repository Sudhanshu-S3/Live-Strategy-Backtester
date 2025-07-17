#ifndef STRATEGY_FACTORY_H
#define STRATEGY_FACTORY_H

#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "../event/ThreadSafeQueue.h"
#include "../data/DataHandler.h"
#include "Strategy.h"
#include "../event/Event.h" // <-- ADD THIS LINE

using EventQueuePtr = std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>>;

class StrategyFactory {
public:
    static std::shared_ptr<Strategy> createStrategy(
        const nlohmann::json& config,
        std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
        std::shared_ptr<DataHandler> data_handler
    );
};

#endif // STRATEGY_FACTORY_H
