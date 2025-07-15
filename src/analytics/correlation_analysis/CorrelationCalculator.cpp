#include "analytics/correlation_analysis/CorrelationCalculator.h"
#include <dlib/statistics.h>

namespace correlation_analysis {

CorrelationCalculator::CorrelationCalculator() {}

matrix_type CorrelationCalculator::calculate_correlation_matrix(const std::vector<std::vector<double>>& returns) {
    if (returns.empty() || returns[0].empty()) {
        return matrix_type();
    }

    size_t num_strategies = returns.size();
    size_t num_returns = returns[0].size();

    matrix_type data(num_returns, num_strategies);
    for (size_t i = 0; i < num_strategies; ++i) {
        for (size_t j = 0; j < num_returns; ++j) {
            data(j, i) = returns[i][j];
        }
    }

    return dlib::correlation(data);
}

} // namespace correlation_analysis
