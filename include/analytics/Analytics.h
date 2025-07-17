#ifndef ANALYTICS_H
#define ANALYTICS_H

#include "nlohmann/json.hpp"
#include "risk/SharpeRatio.h" // <-- CORRECTED PATH
#include "core/Portfolio.h"
#include "../event/Event.h"
#include <memory>
#include <vector>
#include <string>
#include <map>

// Forward declarations if needed
class Portfolio;

class Analytics {
public:
    Analytics(const nlohmann::json& config);

    void generateReport(std::shared_ptr<Portfolio> portfolio);
    void comparePerformance(std::shared_ptr<Portfolio> live_portfolio, std::shared_ptr<Portfolio> backtest_portfolio);
    void generateMarketConditionReport(std::shared_ptr<Portfolio> portfolio);
    void generateFactorAnalysisReport(std::shared_ptr<Portfolio> portfolio);
    void logDeployment(bool success);
    void generateDeploymentReport();
    void snapshotSystemResources();
    void generateResourceUsageReport();
    void detect_anomalies(std::shared_ptr<class DataHandler> data_handler);


private:
    // --- Add these missing member variables ---
    nlohmann::json config_;
    std::unique_ptr<SharpeRatio> sharpe_ratio_;
    int successful_deployments_ = 0;
    int failed_deployments_ = 0;
    bool enable_cross_correlation_ = false;
    
    // For anomaly detection
    std::map<std::string, std::vector<double>> price_history_;
    int anomaly_lookback_ = 50;
    double anomaly_z_score_threshold_ = 3.0;

    // For resource monitoring
    std::vector<size_t> memory_usage_;
    std::vector<double> cpu_usage_;

    void calculate_cross_correlations(std::shared_ptr<Portfolio> portfolio);
};

#endif // ANALYTICS_H