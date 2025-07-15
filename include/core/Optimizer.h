#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "../lib/nlohmann/json.hpp"
#include <string>
#include <vector>
#include <map>
#include <limits> // Added for std::numeric_limits

using json = nlohmann::json;

class Optimizer {
public:
    Optimizer(const json& config);
    json run();
    json getBestParams() const;
    double getBestMetric() const;

private:
    void generate_param_combinations(
        json::const_iterator current_param,
        json::const_iterator end_param,
        json& current_combination,
        std::vector<json>& all_combinations
    );

    json config_;
    json optimization_params_;
    json best_params_;
    double best_metric_ = -std::numeric_limits<double>::infinity();
};

#endif // OPTIMIZER_H 