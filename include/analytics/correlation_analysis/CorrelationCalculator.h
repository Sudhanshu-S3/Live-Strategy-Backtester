#ifndef CORRELATION_CALCULATOR_H
#define CORRELATION_CALCULATOR_H

#include <vector>
#include <dlib/matrix.h>

namespace correlation_analysis {

using matrix_type = dlib::matrix<double>;

class CorrelationCalculator {
public:
    CorrelationCalculator();
    matrix_type calculate_correlation_matrix(const std::vector<std::vector<double>>& returns);
};

} // namespace correlation_analysis

#endif // CORRELATION_CALCULATOR_H
