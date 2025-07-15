#ifndef RISK_MANAGER_H
#define RISK_MANAGER_H

#include "../core/Portfolio.h"
#include "../core/Performance.h" // Include for VaR calculation
#include "../event/Event.h"
#include "../event/ThreadSafeQueue.h"
#include <memory>
#include <string>

struct RiskThresholds {
    double max_drawdown_pct;
    double daily_var_95_pct;
    // New: For circuit breaker
    double portfolio_loss_threshold_pct; 
};

class RiskManager {
public:
    RiskManager(
        std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue, 
        std::shared_ptr<Portfolio> portfolio,
        const nlohmann::json& risk_config
    );

    void onSignal(const SignalEvent& signal);
    void onDataSourceStatus(const DataSourceStatusEvent& event);
    void monitorRealTimeRisk();

private:
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue_;
    std::shared_ptr<Portfolio> portfolio_;
    RiskThresholds thresholds_;

    // Sizing parameters
    bool use_volatility_sizing_;
    double risk_per_trade_pct_;
    int volatility_lookback_;

    // Circuit breaker state
    bool trading_halted_ = false;

    void sendAlert(const std::string& message);
    double calculateVolatility(const std::string& symbol);
};

#endif // RISK_MANAGER_H