#include "analytics/anomaly_detection/AnomalyDetector.h"

namespace anomaly_detection {

AnomalyDetector::AnomalyDetector() {}

void AnomalyDetector::train(const std::vector<sample_type>& normal_samples) {
    trainer_type trainer;
    trainer.set_kernel(kernel_type(0.1));
    df_ = trainer.train(normal_samples);
}

bool AnomalyDetector::is_anomalous(const sample_type& sample) {
    return df_(sample) < 0;
}

} // namespace anomaly_detection
