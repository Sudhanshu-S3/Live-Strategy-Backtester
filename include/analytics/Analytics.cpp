#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <vector>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "core/Portfolio.h"
#include "data/DataHandler.h"

// STAGE 6: New class for advanced analytics
class Analytics {
public:
    Analytics(const nlohmann::json& analytics_config);

    void generateReport(std::shared_ptr<Portfolio> portfolio);
    
    void detect_anomalies(std::shared_ptr<DataHandler> data_handler);

private:
    void calculate_cross_correlations(std::shared_ptr<Portfolio> portfolio);

    bool enable_cross_correlation_;
    double anomaly_z_score_threshold_;
    
    // For anomaly detection
    std::unordered_map<std::string, std::vector<double>> price_history_;
    const int anomaly_lookback_ = 100;
};

#endif