#include "analytics/factor_analysis/FactorExposure.h"
#include <dlib/statistics.h>

namespace factor_analysis {

FactorExposure::FactorExposure(const matrix_type& factor_returns, const matrix_type& asset_returns)
    : factor_returns_(factor_returns), asset_returns_(asset_returns) {}

void FactorExposure::calculate_exposure() {
    // This is a simplified example using linear regression to find the exposure.
    // It assumes that the asset returns can be explained by the factor returns.
    // y = X * beta, where y is asset_returns, X is factor_returns, and beta is the exposure.
    // We can solve for beta using the pseudo-inverse: beta = pinv(X) * y
    exposure_ = dlib::pinv(factor_returns_) * asset_returns_;
}

const matrix_type& FactorExposure::get_exposure() const {
    return exposure_;
}

} // namespace factor_analysis
