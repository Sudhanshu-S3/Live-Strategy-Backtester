#include "../../include/core/WalkForwardAnalyzer.h"
#include "../../include/core/Optimizer.h"
#include "../../include/core/Backtester.h"
#include <iostream>
#include <vector>
#include <string>

// Helper for date manipulation. A proper library would be better for production.
std::string add_months(const std::string& date_str, int months) {
    int year = std::stoi(date_str.substr(0, 4));
    int month = std::stoi(date_str.substr(5, 2));
    
    month += months;
    year += (month - 1) / 12;
    month = (month - 1) % 12 + 1;

    char buffer[11];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-01", year, month);
    return std::string(buffer);
}


WalkForwardAnalyzer::WalkForwardAnalyzer(const json& config) : config_(config) {
    if (config.contains("walk_forward")) {
        walk_forward_params_ = config["walk_forward"];
    }
}

void WalkForwardAnalyzer::run() {
    if (!walk_forward_params_["enabled"].get<bool>()) {
        std::cout << "Walk-forward analysis is disabled in config.json." << std::endl;
        return;
    }

    std::cout << "--- Starting Walk-Forward Analysis ---" << std::endl;

    std::string start_date = walk_forward_params_["data_start_date"];
    std::string end_date = walk_forward_params_["data_end_date"];
    int in_sample_months = walk_forward_params_["in_sample_months"];
    int out_of_sample_months = walk_forward_params_["out_of_sample_months"];

    std::string current_is_start = start_date;

    while (true) {
        std::string current_is_end = add_months(current_is_start, in_sample_months);
        std::string current_oos_start = current_is_end;
        std::string current_oos_end = add_months(current_oos_start, out_of_sample_months);

        if (current_oos_end > end_date) {
            break;
        }

        std::cout << "\n--- Running Period ---" << std::endl;
        std::cout << "In-Sample: " << current_is_start << " to " << current_is_end << std::endl;
        std::cout << "Out-of-Sample: " << current_oos_start << " to " << current_oos_end << std::endl;

        // 1. Run optimization on in-sample data
        json is_config = config_;
        is_config["data"]["start_date"] = current_is_start;
        is_config["data"]["end_date"] = current_is_end;
        is_config["optimization"]["enabled"] = true;
        
        Optimizer is_optimizer(is_config);
        json best_params = is_optimizer.run(); 

        if (best_params.empty()) {
            std::cerr << "Optimization failed for period " << current_is_start << " to " << current_is_end << ". Skipping." << std::endl;
            current_is_start = add_months(current_is_start, out_of_sample_months);
            continue;
        }

        std::cout << "Found best params: " << best_params.dump() << std::endl;

        // 2. Run backtest on out-of-sample data with best params
        json oos_config = config_;
        std::string strategy_to_test = walk_forward_params_["strategy_to_test"];
        for (auto& strategy_config : oos_config["strategies"]) {
            if (strategy_config["name"] == strategy_to_test) {
                strategy_config["params"] = best_params;
                break;
            }
        }
        oos_config["data"]["start_date"] = current_oos_start;
        oos_config["data"]["end_date"] = current_oos_end;

        Backtester oos_backtester(oos_config);
        oos_backtester.run();
        // Here we would collect and aggregate performance from oos_backtester.getPortfolio()

        current_is_start = add_months(current_is_start, out_of_sample_months);
    }

    std::cout << "\n--- Walk-Forward Analysis Complete ---" << std::endl;
} 