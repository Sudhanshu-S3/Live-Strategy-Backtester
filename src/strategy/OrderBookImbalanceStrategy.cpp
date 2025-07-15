#include "../../include/strategy/OrderBookImbalanceStrategy.h"
#include "../../include/data/HFTDataHandler.h"
#include <iostream>
#include <numeric>

// STAGE 3: SIMD for accelerated math
#include <immintrin.h>

OrderBookImbalanceStrategy::OrderBookImbalanceStrategy(
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
    std::shared_ptr<DataHandler> data_handler,
    const std::string& symbol,
    int lookback_levels,
    double imbalance_threshold
) : Strategy(event_queue, data_handler, symbol, "ORDER_BOOK_IMBALANCE"),
    lookback_levels_(lookback_levels),
    imbalance_threshold_(imbalance_threshold) 
{
    // STAGE 3: Pre-allocate aligned memory for SIMD
    bid_prices_f.resize(lookback_levels);
    bid_qtys_f.resize(lookback_levels);
    ask_prices_f.resize(lookback_levels);
    ask_qtys_f.resize(lookback_levels);
}

void OrderBookImbalanceStrategy::calculate_signals() {
    if (!is_active_) return;

    auto hft_data_handler = std::dynamic_pointer_cast<HFTDataHandler>(data_handler_);
    if (!hft_data_handler) return;

    OrderBook book = hft_data_handler->getLatestOrderBook(symbol_);
    if (book.bids.empty() || book.asks.empty()) {
        return;
    }

    // STAGE 3: Incremental computation - only calculate on new book data
    if (book.timestamp == last_update_timestamp_) {
        return;
    }
    last_update_timestamp_ = book.timestamp;

    // --- Standard C++ calculation ---
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

    // --- STAGE 3: SIMD-accelerated calculation (example) ---
    // This is a simplified example. A real use case would be more complex,
    // e.g., calculating VWAP or other weighted metrics.
    int n = std::min((int)book.bids.size(), lookback_levels_);
    // Copy data to float vectors...
    
    __m256 bid_sum_vec = _mm256_setzero_ps();
    for (int i = 0; i < n; i += 8) {
        if (i + 8 <= n) {
            __m256 qtys = _mm256_loadu_ps(&bid_qtys_f[i]);
            bid_sum_vec = _mm256_add_ps(bid_sum_vec, qtys);
        }
    }
    // Horizontal add to get the final sum
    // float total_bid_volume_simd = ...;


    if (total_ask_volume > 1e-9) {
        double imbalance = total_bid_volume / total_ask_volume;
        if (imbalance > imbalance_threshold_) {
            // Strong buying pressure
            generate_signal(OrderDirection::BUY, 1.0);
        } else if (imbalance < (1.0 / imbalance_threshold_)) {
            // Strong selling pressure
            generate_signal(OrderDirection::SELL, 1.0);
        }
    }
}