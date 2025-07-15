#include "../../include/analytics/Analytics.h"
#include <iostream>
#include <numeric>
#include <cmath>

Analytics::Analytics(const nlohmann::json& analytics_config) {
    enable_cross_correlation_ = analytics_config.value("enable_cross_correlation", false);
    anomaly_z_score_threshold_ = analytics_config.value("anomaly_detection_z_score", 3.0);
}

void Analytics::generateReport(std::shared_ptr<Portfolio> portfolio) {
    std::cout << "\n--- Advanced Analytics Report ---\n";
    if (enable_cross_correlation_) {
        calculate_cross_correlations(portfolio);
    }
    std::cout << "---------------------------------\n";
}

void Analytics::calculate_cross_correlations(std::shared_ptr<Portfolio> portfolio) {
    // This is a placeholder. A real implementation would need equity curves
    // for each individual strategy, which requires enhancing the Portfolio class
    // to track them separately.
    std::cout << "Cross-Strategy Correlation Analysis:" << std::endl;
    std::cout << "(Conceptual) To implement this, the Portfolio would need to track an equity curve per strategy." << std::endl;
}

void Analytics::detect_anomalies(std::shared_ptr<DataHandler> data_handler) {
    if (anomaly_z_score_threshold_ <= 0) return;

    for(const auto& symbol : data_handler->getSymbols()) {
        double price = data_handler->getLatestBarValue(symbol, "price");
        if(price <= 0) continue;

        price_history_[symbol].push_back(price);
        if(price_history_[symbol].size() > anomaly_lookback_) {
            price_history_[symbol].erase(price_history_[symbol].begin());
        }
        
        if(price_history_[symbol].size() < anomaly_lookback_) continue;

        // Calculate z-score for the latest price
        double sum = std::accumulate(price_history_[symbol].begin(), price_history_[symbol].end(), 0.0);
        double mean = sum / anomaly_lookback_;
        double sq_sum = std::inner_product(price_history_[symbol].begin(), price_history_[symbol].end(), price_history_[symbol].begin(), 0.0);
        double std_dev = std::sqrt(sq_sum / anomaly_lookback_ - mean * mean);

        if(std_dev > 1e-9){
            double z_score = (price - mean) / std_dev;
            if(std::abs(z_score) > anomaly_z_score_threshold_){
                std::cerr << "!!! MARKET ANOMALY DETECTED !!! Symbol: " << symbol 
                          << ", Price: " << price << ", Z-Score: " << z_score << std::endl;
            }
        }
    }
}