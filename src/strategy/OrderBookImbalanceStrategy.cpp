#include "../../include/strategy/OrderBookImbalanceStrategy.h"
#include "../../include/data/HFTDataHandler.h"
#include <iostream>
#include <numeric>
<<<<<<< HEAD
#include <algorithm> // For std::min
=======
>>>>>>> ef82a6ae559d39c2be7a0dee4c6355537669c2a5

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
<<<<<<< HEAD
    // Pre-allocate aligned memory for SIMD
=======
    // STAGE 3: Pre-allocate aligned memory for SIMD
>>>>>>> ef82a6ae559d39c2be7a0dee4c6355537669c2a5
    bid_prices_f.resize(lookback_levels);
    bid_qtys_f.resize(lookback_levels);
    ask_prices_f.resize(lookback_levels);
    ask_qtys_f.resize(lookback_levels);
<<<<<<< HEAD
    
    // Add state tracking
    last_signal_time_ = 0;
    signal_cooldown_ms_ = 10000; // 10 seconds cooldown between signals
    current_position_ = PositionState::FLAT;
    
    std::cout << "OrderBookImbalanceStrategy initialized with lookback_levels=" 
              << lookback_levels_ << ", imbalance_threshold=" 
              << base_imbalance_threshold_ << std::endl;
=======
>>>>>>> ef82a6ae559d39c2be7a0dee4c6355537669c2a5
}

void OrderBookImbalanceStrategy::onMarket(const MarketEvent& event) {
    // Not used for this strategy
}

void OrderBookImbalanceStrategy::onTrade(const TradeEvent& event) {
    // Not used for this strategy
}

<<<<<<< HEAD
void OrderBookImbalanceStrategy::onOrderBook(const OrderBookEvent& event) {
    // Fix 1: Use symbol_ from event instead of symbol
    if (event.symbol_ != getSymbol()) {
        return;  // Skip events for other symbols
    }

    // Fix 2: Use bid_levels_ and ask_levels_ instead of bids and asks
    const auto& bids = event.bid_levels_;
    const auto& asks = event.ask_levels_;

    // Check if we have enough data to calculate imbalances
    if (bids.size() < 2 || asks.size() < 2) {
        return;  // Not enough levels to analyze
    }

    // Fix 3: Fix std::min usage - nested calls instead of initializer list
    int used_levels = std::min(lookback_levels_, 
                              std::min(static_cast<int>(bids.size()),
                                      static_cast<int>(asks.size())));
    
    // Prepare data for processing
    for (int i = 0; i < used_levels; ++i) {
        bid_prices_f[i] = static_cast<float>(bids[i].price);
        bid_qtys_f[i] = static_cast<float>(bids[i].quantity);
        ask_prices_f[i] = static_cast<float>(asks[i].price);
        ask_qtys_f[i] = static_cast<float>(asks[i].quantity);
    }

    // Calculate total bid and ask quantities for top levels
    float total_bid_qty = 0.0f;
    float total_ask_qty = 0.0f;
    
    for (int i = 0; i < used_levels; ++i) {
        total_bid_qty += bid_qtys_f[i];
        total_ask_qty += ask_qtys_f[i];
    }
    
    // Calculate order book imbalance
    float imbalance_ratio = 0.0f;
    if (total_ask_qty > 0.0f) {
        imbalance_ratio = total_bid_qty / total_ask_qty;
    }
    
    // Get current time for signal cooldown check
    auto now = std::chrono::high_resolution_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    // Log the imbalance periodically
    if (now_ms % 5000 < 100) { // Log roughly every 5 seconds
        // Fix 4: Use symbol_ from event
        std::cout << "ORDER BOOK IMBALANCE: " << event.symbol_ 
                << " | Ratio: " << imbalance_ratio 
                << " | Threshold: " << base_imbalance_threshold_ 
                << " | Position: " << (current_position_ == PositionState::LONG ? "LONG" : 
                                     current_position_ == PositionState::SHORT ? "SHORT" : "FLAT")
                << std::endl;
    }
    
    // Only generate new signals if enough time has passed since the last one
    if (now_ms - last_signal_time_ < signal_cooldown_ms_) {
        return;
    }
    
    // Generate trading signals based on imbalance
    if (imbalance_ratio > base_imbalance_threshold_ && current_position_ != PositionState::LONG) {
        // Strong buying pressure - generate BUY signal
        generate_signal(OrderDirection::BUY);
        current_position_ = PositionState::LONG;
        last_signal_time_ = now_ms;
    } 
    else if (imbalance_ratio < (1.0f / base_imbalance_threshold_) && current_position_ != PositionState::SHORT) {
        // Strong selling pressure - generate SELL signal
        generate_signal(OrderDirection::SELL);
        current_position_ = PositionState::SHORT;
        last_signal_time_ = now_ms;
    }
    // Optional: Add mean-reversion condition to close positions
    else if (imbalance_ratio >= 0.9f && imbalance_ratio <= 1.1f && current_position_ != PositionState::FLAT) {
        // Close position when imbalance neutralizes
        OrderDirection close_direction = (current_position_ == PositionState::LONG) ? 
                                        OrderDirection::SELL : OrderDirection::BUY;
        generate_signal(close_direction);
        current_position_ = PositionState::FLAT;
        last_signal_time_ = now_ms;
    }
}

void OrderBookImbalanceStrategy::onFill(const FillEvent& event) {
    // Update position state based on fills
    if (event.direction == OrderDirection::BUY) {
        std::cout << "Fill received: BUY " << event.quantity << " " << event.symbol << " @ " << event.fill_price << std::endl;
    } else {
        std::cout << "Fill received: SELL " << event.quantity << " " << event.symbol << " @ " << event.fill_price << std::endl;
    }
}

void OrderBookImbalanceStrategy::generate_signal(OrderDirection direction) {
    long long timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    
    // Fix 5: Use getName() and getSymbol() from base class
    auto signal = std::make_shared<SignalEvent>(getName(), getSymbol(), timestamp, direction, 0.0, 1.0);
    
    // Fix 6: Wrap signal in another shared_ptr to match the queue's expected type
    event_queue_->push(std::make_shared<std::shared_ptr<Event>>(signal));
    
    std::cout << "\n🚨 SIGNAL GENERATED: " << getSymbol()
              << " | Direction: " << (direction == OrderDirection::BUY ? "BUY" : "SELL")
              << " | Time: " << timestamp << std::endl;
=======
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
>>>>>>> ef82a6ae559d39c2be7a0dee4c6355537669c2a5
}