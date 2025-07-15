#include "../../include/core/MonteCarloSimulator.h"
#include "../../include/core/Backtester.h"
#include <iostream>
#include <random>
#include <vector>

MonteCarloSimulator::MonteCarloSimulator(const json& config) : config_(config) {
    if (config.contains("monte_carlo")) {
        mc_params_ = config["monte_carlo"];
    }
}

void MonteCarloSimulator::run(int num_simulations) {
    if (!mc_params_.is_object() || !mc_params_.value("enabled", false)) {
        std::cout << "Monte Carlo simulation is disabled or not configured." << std::endl;
        return;
    }

    std::cout << "--- Starting Monte Carlo Simulation ---" << std::endl;
    std::cout << "Number of simulations: " << num_simulations << std::endl;

    json base_params = mc_params_["base_params"];
    std::string strategy_to_test = mc_params_["strategy_to_test"];
    
    std::vector<double> results;
    std::default_random_engine generator;

    for (int i = 0; i < num_simulations; ++i) {
        json randomized_params = base_params;
        
        // Randomize parameters
        for (auto& item : mc_params_["randomization_ranges"].items()) {
            std::string param_name = item.key();
            double min_val = item.value()[0];
            double max_val = item.value()[1];
            std::uniform_real_distribution<double> distribution(min_val, max_val);
            
            if (randomized_params[param_name].is_number_integer()) {
                randomized_params[param_name] = static_cast<int>(distribution(generator));
            } else {
                randomized_params[param_name] = distribution(generator);
            }
        }
        
        std::cout << "\nRunning simulation " << i + 1 << " with params: " << randomized_params.dump() << std::endl;
        
        json run_config = config_;
        for (auto& strategy_config : run_config["strategies"]) {
            if (strategy_config["name"] == strategy_to_test) {
                strategy_config["params"] = randomized_params;
                break;
            }
        }

        Backtester backtester(run_config);
        backtester.run();
        double result = backtester.getPortfolio()->getRealTimePerformance().getSharpeRatio();
        results.push_back(result);
        std::cout << "Resulting Sharpe Ratio: " << result << std::endl;
    }

    std::cout << "\n--- Monte Carlo Simulation Complete ---" << std::endl;
    // Basic analysis of results
    double sum = std::accumulate(results.begin(), results.end(), 0.0);
    double mean = sum / results.size();
    double sq_sum = std::inner_product(results.begin(), results.end(), results.begin(), 0.0);
    double stddev = std::sqrt(sq_sum / results.size() - mean * mean);

    std::cout << "Sharpe Ratio Stats:" << std::endl;
    std::cout << "  Mean: " << mean << std::endl;
    std::cout << "  Std Dev: " << stddev << std::endl;
    std::cout << "  Min: " << *std::min_element(results.begin(), results.end()) << std::endl;
    std::cout << "  Max: " << *std::max_element(results.begin(), results.end()) << std::endl;
    std::cout << "---------------------------------------" << std::endl;
} 