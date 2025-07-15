#include "analytics/forecasting/PerformanceForecasting.h"
#include <stdexcept>

namespace forecasting {

PerformanceForecaster::PerformanceForecaster() {
    // The trainer can be configured here. For example, setting the C parameter.
    // trainer.set_c(10);
}

void PerformanceForecaster::train(const std::vector<sample_type>& samples, const std::vector<double>& labels) {
    if (samples.empty() || labels.empty()) {
        throw std::invalid_argument("Training samples or labels cannot be empty.");
    }
    if (samples.size() != labels.size()) {
        throw std::invalid_argument("The number of samples must match the number of labels.");
    }

    // The dlib SVM trainer takes the training data and returns a decision function.
    trainer_type trainer;
    df_ = trainer.train(samples, labels);
}

double PerformanceForecaster::predict(const sample_type& sample) {
    // Check if the model has been trained.
    // A default-constructed decision_function is not "good"
    if (df_.basis_vectors.size() == 0) {
        throw std::runtime_error("Model has not been trained yet. Call train() before is_anomalous().");
    }
    
    // Use the learned decision function to predict the value for a new sample.
    return df_(sample);
}

} // namespace forecasting
