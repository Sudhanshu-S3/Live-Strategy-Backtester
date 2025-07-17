#ifndef BACKTESTER_H
#define BACKTESTER_H

#include <vector>
#include <string>
#include <memory>
#include <queue>
#include <thread>
#include <chrono>
#include <atomic>
#include <nlohmann/json.hpp>
#include <unordered_map>

#include "event/Event.h"
#include "event/ThreadSafeQueue.h"
#include "data/DataHandler.h"
#include "strategy/Strategy.h"
#include "core/Portfolio.h"
#include "execution/ExecutionHandler.h"
#include "risk/RiskManager.h"
#include "analytics/Analytics.h"
#include "strategy/MarketRegimeDetector.h"
#include "strategy/MLStrategyClassifier.h"
#include "analytics/PerformanceForecaster.h"
#include "CustomAllocator.h"

enum class RunMode { BACKTEST, SHADOW, OPTIMIZATION, WALK_FORWARD };

class Backtester {
public:
    Backtester(
        const std::vector<std::string>& symbols,
        const std::string& csv_dir,
        const std::string& start_date,
        const std::string& end_date,
        double initial_capital,
        std::unique_ptr<Strategy> strategy
    );
    Backtester(const nlohmann::json& config);
    ~Backtester();

    void run();

    const nlohmann::json& config() const { return config_; }
    RunMode run_mode() const { return run_mode_; }
    std::shared_ptr<Portfolio> getPortfolio() const { return portfolio_; }

private:
    void run_backtest();
    nlohmann::json run_optimization();
    void run_walk_forward();
    void main_loop();
    void handle_market_event(const std::shared_ptr<Event>& event);
    void handle_signal_event(const std::shared_ptr<Event>& event);
    void handle_order_event(const std::shared_ptr<Event>& event);
    void handle_fill_event(const std::shared_ptr<Event>& event);

    void start_strategy_threads();
    void strategy_thread_worker(std::shared_ptr<Strategy> strategy);
    void handleEvent(const std::shared_ptr<Event>& event);
    void log_live_performance();

    nlohmann::json config_;
    RunMode run_mode_;
    
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue_;
    std::shared_ptr<DataHandler> data_handler_;
    std::vector<std::shared_ptr<Strategy>> strategies_;
    std::shared_ptr<Portfolio> portfolio_;
    std::shared_ptr<ExecutionHandler> execution_handler_;
    std::shared_ptr<RiskManager> risk_manager_;
    std::shared_ptr<Analytics> analytics_;
    std::unique_ptr<MLStrategyClassifier> strategy_classifier_;
    std::unique_ptr<PerformanceForecaster> performance_forecaster_;
    std::shared_ptr<MarketRegimeDetector> market_regime_detector_; // <-- FIX: Added this member

    std::atomic<bool> continue_backtest_{true};
    std::vector<std::thread> strategy_threads_;

    // For live monitoring
    std::chrono::steady_clock::time_point last_monitor_time_;
    long long monitor_interval_ms_;

    // For risk monitoring
    std::chrono::steady_clock::time_point last_risk_check_time_;
    long long risk_check_interval_ms_;

    // For resource monitoring
    std::chrono::steady_clock::time_point last_resource_check_time_;
    long long resource_check_interval_ms_;

    // Using custom allocator for time series data
    using BarVector = std::vector<Bar, PoolAllocator<Bar>>;
    std::unordered_map<std::string, BarVector> time_series_data_;
};

#endif // BACKTESTER_H