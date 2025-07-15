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
    if (event.symbol != symbol) return;

    const OrderBook& book = event.book;

    if (book.bids.empty() || book.asks.empty()) {
        return;
    }

    if (book.timestamp == last_update_timestamp_) {
        return;
    }
    last_update_timestamp_ = book.timestamp;

    // Adjust threshold based on volatility
    double current_imbalance_threshold = base_imbalance_threshold_;
    if (current_market_state_.volatility == VolatilityLevel::HIGH) {
        current_imbalance_threshold *= 1.5; // Require a stronger signal in high vol
    } else if (current_market_state_.volatility == VolatilityLevel::LOW) {
        current_imbalance_threshold *= 0.8; // Be more sensitive in low vol
    }

    double total_bid_volume = 0;
    auto bid_it = book.bids.rbegin();
    for(int i = 0; i < lookback_levels_ && bid_it != book.bids.rend(); ++i, ++bid_it) {
        total_bid_volume += bid_it->second;
    }

    double total_ask_volume = 0;
    auto ask_it = book.asks.begin();
    for(int i = 0; i < lookback_levels_ && ask_it != book.asks.end(); ++i, ++ask_it) {
        total_ask_volume += ask_it->second;
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