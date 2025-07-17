#ifndef PERFORMANCE_FORECASTING_H
#define PERFORMANCE_FORECASTING_H

#include <vector>
#include "data/DataTypes.h"
#include <dlib/svm.h>

namespace forecasting {

using sample_type = dlib::matrix<double, 0, 1>;
using kernel_type = dlib::linear_kernel<sample_type>;
using trainer_type = dlib::svm_regression_trainer<kernel_type>;

class PerformanceForecaster {
public:
    PerformanceForecaster();
    void train(const std::vector<sample_type>& samples, const std::vector<double>& labels);
    double predict(const sample_type& sample);

private:
    dlib::decision_function<kernel_type> df_;
};

} // namespace forecasting

#endif // PERFORMANCE_FORECASTING_H
