#include "analytics/anomaly_detection/AnomalyDetector.h"
#include <stdexcept>

namespace anomaly_detection {

AnomalyDetector::AnomalyDetector() {
    // The trainer can be configured here. For example, setting nu and gamma.
    // trainer.set_nu(0.1);
    // trainer.set_kernel(kernel_type(0.1));
}

void AnomalyDetector::train(const std::vector<sample_type>& normal_samples) {
    if (normal_samples.empty()) {
        throw std::invalid_argument("Training samples cannot be empty.");
    }

    // The one-class SVM trainer learns a boundary around the "normal" data.
    trainer_type trainer;
    df_ = trainer.train(normal_samples);
}

bool AnomalyDetector::is_anomalous(const sample_type& sample) {
    // Check if the model has been trained.
    if (df_.basis_vectors.size() == 0) {
        throw std::runtime_error("Model has not been trained yet. Call train() before is_anomalous().");
    }

    // The decision function returns a value > 0 for "normal" samples
    // and < 0 for anomalous ones.
    return df_(sample) < 0;
}

} // namespace anomaly_detection
