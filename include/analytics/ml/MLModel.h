#ifndef ML_MODEL_H
#define ML_MODEL_H

#include <dlib/svm.h>
#include <vector>

namespace ml {

using sample_type = dlib::matrix<double, 0, 1>;
using kernel_type = dlib::radial_basis_kernel<sample_type>;
using trainer_type = dlib::svm_c_trainer<kernel_type>;

class StrategyClassifier {
public:
    StrategyClassifier();
    void train(const std::vector<sample_type>& samples, const std::vector<double>& labels);
    double predict(const sample_type& sample);

private:
    dlib::decision_function<kernel_type> df_;
};

} // namespace ml

#endif // ML_MODEL_H
