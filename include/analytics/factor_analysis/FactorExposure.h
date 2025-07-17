#ifndef FACTOR_EXPOSURE_H
#define FACTOR_EXPOSURE_H

#include <vector>
#include <dlib/matrix.h>

namespace factor_analysis {

using matrix_type = dlib::matrix<double>;

class FactorExposure {
public:
    FactorExposure(const matrix_type& factor_returns, const matrix_type& asset_returns);
    void calculate_exposure();
    const matrix_type& get_exposure() const;

private:
    matrix_type factor_returns_;
    matrix_type asset_returns_;
    matrix_type exposure_;
};

} // namespace factor_analysis

#endif // FACTOR_EXPOSURE_H
