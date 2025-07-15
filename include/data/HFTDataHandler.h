#ifndef HFT_DATA_HANDLER_H
#define HFT_DATA_HANDLER_H

#include "data/DataHandler.h"
#include "data/DataTypes.h"
#include <fstream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <cmath>

// STAGE 3: High-performance spinlock to replace std::mutex for short critical sections
class Spinlock {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
public:
    void lock() { while (flag.test_and_set(std::memory_order_acquire)); }
    void unlock() { flag.clear(std::memory_order_release); }
};


class HFTDataHandler : public DataHandler {
public:
    HFTDataHandler(std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue,
                   const std::vector<std::string>& symbols,
                   const std::string& trade_data_dir,
                   const std::string& book_data_dir,
                   const std::string& historical_data_fallback_dir);

    virtual ~HFTDataHandler() = default;

    void updateBars(std::queue<std::shared_ptr<Event>>& event_queue) override;
    bool isFinished() const override;
    std::optional<Bar> getLatestBar(const std::string& symbol) const override;
    double getLatestBarValue(const std::string& symbol, const std::string& val_type) override;
    std::vector<Bar> getLatestBars(const std::string& symbol, int n = 1) override; 

    // STAGE 3: Accessors for strategies running in other threads
    OrderBook getLatestOrderBook(const std::string& symbol);
    Trade getLatestTrade(const std::string& symbol);

    void connectLiveFeed();
    bool isLive() const { return is_live_feed_.load(); }
    bool isConnected() const { return is_connected_.load(); }

private:
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue_;
    std::vector<std::string> symbols_;
    std::string trade_data_dir_;
    std::string book_data_dir_;
    std::string historical_data_fallback_dir_;

    std::unordered_map<std::string, std::vector<Trade>> all_trades_;
    std::unordered_map<std::string, std::vector<OrderBook>> all_orderbooks_;

    std::unordered_map<std::string, size_t> trade_indices_;
    std::unordered_map<std::string, size_t> orderbook_indices_;
    
    mutable Spinlock data_spinlock_; // STAGE 3: Using spinlock
    std::atomic<bool> is_live_feed_ = {false};

    std::atomic<bool> is_connected_ = {false};
    std::atomic<int> connection_retries_ = {0};
    const int max_connection_retries_ = 5;
    const int base_retry_delay_ms_ = 1000;
    void attemptReconnection();
    void fallbackToHistoricalData();
    bool historical_fallback_active_ = false;

    bool load_data(const std::string& symbol, const std::string& dir);
};

#endif