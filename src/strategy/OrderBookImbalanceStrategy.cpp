#include "../../include/strategy/OrderBookImbalanceStrategy.h"
#include "../../include/data/HFTDataHandler.h"
#include <iostream>
#include <numeric>

// STAGE 3: SIMD for accelerated math
#include <immintrin.h>

OrderBookImbalanceStrategy::OrderBookImbalanceStrategy(
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
    std::shared_ptr<DataHandler> data_handler,
    const std::string& symbol,
    int lookback_levels,
    double imbalance_threshold
) : Strategy(event_queue, data_handler, "ORDER_BOOK_IMBALANCE", symbol),
    lookback_levels_(lookback_levels),
    base_imbalance_threshold_(imbalance_threshold) 
{
    // STAGE 3: Pre-allocate aligned memory for SIMD
    bid_prices_f.resize(lookback_levels);
    bid_qtys_f.resize(lookback_levels);
    ask_prices_f.resize(lookback_levels);
    ask_qtys_f.resize(lookback_levels);
}

void OrderBookImbalanceStrategy::onMarket(const MarketEvent& event) {
    // Not used for this strategy
}

void OrderBookImbalanceStrategy::onTrade(const TradeEvent& event) {
    // Not used for this strategy
}

void OrderBookImbalanceStrategy::onFill(const FillEvent& event) {
    // Can be used to adjust strategy behavior based on fills
}

void OrderBookImbalanceStrategy::onMarketRegimeChanged(const MarketRegimeChangedEvent& event) {
    current_market_state_ = event.new_state;
}

void OrderBookImbalanceStrategy::onOrderBook(const OrderBookEvent& event) {
    // Fix accessing symbol_ instead of symbol
    if (event.symbol_ != symbol) return;
    
    // There's no book member, we need to use the OrderBookEvent directly
    // The bid_levels_ and ask_levels_ contain the order book data
    
    // Get the bid and ask levels from the event
    const auto& bid_levels = event.bid_levels_;
    const auto& ask_levels = event.ask_levels_;
    
    // Use the bid and ask levels directly instead of trying to access event.book
    // Rest of your implementation...
    
    // Example implementation:
    if (bid_levels.empty() || ask_levels.empty()) return;
    
    // Copy data for SIMD processing
    size_t bid_size = std::min(static_cast<size_t>(lookback_levels_), bid_levels.size());
    size_t ask_size = std::min(static_cast<size_t>(lookback_levels_), ask_levels.size());
    
    for (size_t i = 0; i < bid_size; i++) {
        bid_prices_f[i] = static_cast<float>(bid_levels[i].price);
        bid_qtys_f[i] = static_cast<float>(bid_levels[i].quantity);
    }
    
    for (size_t i = 0; i < ask_size; i++) {
        ask_prices_f[i] = static_cast<float>(ask_levels[i].price);
        ask_qtys_f[i] = static_cast<float>(ask_levels[i].quantity);
    }
    
    // Calculate imbalance and generate signals as needed
    double total_bid_volume = std::accumulate(bid_qtys_f.begin(), bid_qtys_f.begin() + bid_size, 0.0);
    double total_ask_volume = std::accumulate(ask_qtys_f.begin(), ask_qtys_f.begin() + ask_size, 0.0);

    // Adjust threshold based on volatility
    double current_imbalance_threshold = base_imbalance_threshold_;
    if (current_market_state_.volatility == VolatilityLevel::HIGH) {
        current_imbalance_threshold *= 1.5; // Require a stronger signal in high vol
    } else if (current_market_state_.volatility == VolatilityLevel::LOW) {
        current_imbalance_threshold *= 0.8; // Be more sensitive in low vol
    }

    if (total_ask_volume > 1e-9) {
        double imbalance = total_bid_volume / total_ask_volume;
        if (imbalance > current_imbalance_threshold) {
            generate_signal(OrderDirection::BUY, 1.0);
        } else if (imbalance < (1.0 / current_imbalance_threshold)) {
            generate_signal(OrderDirection::SELL, 1.0);
        }
    }
}

void OrderBookImbalanceStrategy::generate_signal(OrderDirection direction, double strength) {
    long long timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    // For this strategy, let's assume no specific stop loss is calculated here,
    // it could be handled by the RiskManager.
    double stop_loss = 0.0; 
    auto signal = std::make_shared<SignalEvent>(name, symbol, timestamp, direction, stop_loss, strength);
    event_queue_->push(std::make_shared<std::shared_ptr<Event>>(signal));
}