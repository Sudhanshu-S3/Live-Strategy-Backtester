#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>
#include "core/Portfolio.h"
#include "data/DataHandler.h"

// STAGE 6: New class for advanced analytics
class Analytics {
public:
    Analytics(const nlohmann::json& analytics_config);

    void generateReport(std::shared_ptr<Portfolio> portfolio);
    
    void comparePerformance(std::shared_ptr<Portfolio> live_portfolio, std::shared_ptr<Portfolio> backtest_portfolio);
    void generateMarketConditionReport(std::shared_ptr<Portfolio> portfolio);
    void generateFactorAnalysisReport(std::shared_ptr<Portfolio> portfolio);

    void detect_anomalies(std::shared_ptr<DataHandler> data_handler);

    void logLivePerformance(std::shared_ptr<Portfolio> portfolio);

    // For deployment tracking
    void logDeployment(bool success);
    void generateDeploymentReport();

    // For system resource tracking
    void snapshotSystemResources();
    void generateResourceUsageReport();

private:
    void calculate_cross_correlations(std::shared_ptr<Portfolio> portfolio);

    bool enable_cross_correlation_;
    double anomaly_z_score_threshold_;
    
    // For anomaly detection
    std::unordered_map<std::string, std::vector<double>> price_history_;
    const int anomaly_lookback_ = 100;

    // For live logging
    std::chrono::steady_clock::time_point last_log_time_;

    // For deployment tracking
    int successful_deployments_ = 0;
    int failed_deployments_ = 0;

    // For resource usage
    std::vector<double> cpu_usage_;
    std::vector<size_t> memory_usage_;
};

#endif