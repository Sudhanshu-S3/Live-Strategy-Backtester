#ifndef HFT_DATA_HANDLER_H
#define HFT_DATA_HANDLER_H

#include "data/DataHandler.h"
#include "data/DataTypes.h"
#include "../event/ThreadSafeQueue.h"
#include <fstream>
#include <unordered_map>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <cmath>
#include <mutex>
#include <condition_variable>

// STAGE 3: High-performance spinlock to replace std::mutex for short critical sections
class Spinlock {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
public:
    void lock() { while (flag.test_and_set(std::memory_order_acquire)); }
    void unlock() { flag.clear(std::memory_order_release); }
};


class HFTDataHandler : public DataHandler {
public:
    HFTDataHandler(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
                   const std::vector<std::string>& symbols,
                   const std::string& trade_data_dir,
                   const std::string& book_data_dir,
                   const std::string& historical_data_fallback_dir,
                   const std::string& start_date = "", // YYYY-MM-DD
                   const std::string& end_date = ""    // YYYY-MM-DD
                   );

    virtual ~HFTDataHandler() = default;

    void updateBars() override;
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

    // --- New for event-driven strategies ---
    std::mutex& getDataMutex() { return data_notification_mutex_; }
    std::condition_variable& getDataCondition() { return data_notification_cond_; }
    void notifyNewData() { data_notification_cond_.notify_all(); }

private:
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue_;
    std::vector<std::string> symbols_;
    std::string trade_data_dir_;
    std::string book_data_dir_;
    std::string historical_data_fallback_dir_;

    std::unordered_map<std::string, std::vector<Trade>> all_trades_;
    std::unordered_map<std::string, std::vector<OrderBook>> all_orderbooks_;

    std::unordered_map<std::string, size_t> trade_indices_;
    std::unordered_map<std::string, size_t> orderbook_indices_;
    
    mutable Spinlock data_spinlock_; // STAGE 3: Using spinlock

    // --- For signaling strategies ---
    mutable std::mutex data_notification_mutex_;
    std::condition_variable data_notification_cond_;

    std::atomic<bool> is_live_feed_ = {false};

    std::atomic<bool> is_connected_ = {false};
    std::atomic<int> connection_retries_ = {0};
    const int max_connection_retries_ = 5;
    const int base_retry_delay_ms_ = 1000;
    void attemptReconnection();
    void fallbackToHistoricalData();
    bool historical_fallback_active_ = false;

    bool load_data(const std::string& symbol, const std::string& dir, const std::string& start_date, const std::string& end_date);
};

#endif