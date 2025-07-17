#ifndef MONTE_CARLO_SIMULATOR_H
#define MONTE_CARLO_SIMULATOR_H

#include <nlohmann/json.hpp>
#include <vector>

using json = nlohmann::json;

class MonteCarloSimulator {
public:
    MonteCarloSimulator(const json& config);
    void run(int num_simulations);

private:
    json config_;
    json mc_params_;
};

#endif // MONTE_CARLO_SIMULATOR_H 