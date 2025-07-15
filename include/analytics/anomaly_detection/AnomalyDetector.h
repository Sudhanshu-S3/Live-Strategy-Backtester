#ifndef ANOMALY_DETECTION_H
#define ANOMALY_DETECTION_H

#include <vector>
#include "data/DataTypes.h"
#include <dlib/svm.h>

namespace anomaly_detection {

using sample_type = dlib::matrix<double, 0, 1>;
using kernel_type = dlib::one_class_svm_kernel<dlib::radial_basis_kernel<sample_type>>;
using trainer_type = dlib::svm_one_class_trainer<kernel_type>;

class AnomalyDetector {
public:
    AnomalyDetector();
    void train(const std::vector<sample_type>& normal_samples);
    bool is_anomalous(const sample_type& sample);

private:
    dlib::decision_function<kernel_type> df_;
};

} // namespace anomaly_detection

#endif // ANOMALY_DETECTION_H
