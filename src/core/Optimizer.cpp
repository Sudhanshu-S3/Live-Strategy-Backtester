#include "../../include/core/Optimizer.h"
#include "../../include/core/Backtester.h"
#include "../../include/core/Portfolio.h"
#include <iostream>

Optimizer::Optimizer(const json& config) : config_(config) {
    if (config.contains("optimization")) {
        optimization_params_ = config["optimization"];
    }
}

json Optimizer::run() {
    if (!optimization_params_.is_object() || !optimization_params_.value("enabled", false)) {
        std::cout << "Optimization is disabled or not configured." << std::endl;
        return json{};
    }

    std::cout << "--- Starting Strategy Optimization ---" << std::endl;

    std::vector<json> param_combinations;
    json current_combination;
    generate_param_combinations(
        optimization_params_["param_ranges"].begin(),
        optimization_params_["param_ranges"].end(),
        current_combination,
        param_combinations
    );

    std::cout << "Generated " << param_combinations.size() << " parameter combinations." << std::endl;

    double best_metric_ = -std::numeric_limits<double>::infinity();
    json best_params_;

    for (const auto& params : param_combinations) {
        std::cout << "\nTesting parameters: " << params.dump() << std::endl;

        json run_config = config_;
        std::string strategy_to_optimize = optimization_params_["strategy_to_optimize"];
        
        for (auto& strategy_config : run_config["strategies"]) {
            if (strategy_config["name"] == strategy_to_optimize) {
                strategy_config["params"] = params;
                break;
            }
        }
        
        Backtester backtester(run_config);
        backtester.run();
        
        std::shared_ptr<Portfolio> portfolio = backtester.getPortfolio();
        double current_metric = portfolio->getRealTimePerformance().getSharpeRatio();

        std::cout << "Resulting Sharpe Ratio: " << current_metric << std::endl;

        if (current_metric > best_metric_) {
            best_metric_ = current_metric;
            best_params_ = params;
        }
    }

    std::cout << "\n--- Optimization Complete ---" << std::endl;
    std::cout << "Best parameters found: " << best_params_.dump() << std::endl;
    std::cout << "Best Sharpe Ratio: " << best_metric_ << std::endl;
    std::cout << "--------------------------" << std::endl;

    return best_params_;
}

json Optimizer::getBestParams() const {
    return best_params_;
}

double Optimizer::getBestMetric() const {
    return best_metric_;
}

void Optimizer::generate_param_combinations(
    json::const_iterator current_param,
    json::const_iterator end_param,
    json& current_combination,
    std::vector<json>& all_combinations
) {
    if (current_param == end_param) {
        all_combinations.push_back(current_combination);
        return;
    }

    const std::string& param_name = current_param.key();
    const json& values = current_param.value();

    for (const auto& value : values) {
        current_combination[param_name] = value;
        generate_param_combinations(current_param + 1, end_param, current_combination, all_combinations);
    }
} 