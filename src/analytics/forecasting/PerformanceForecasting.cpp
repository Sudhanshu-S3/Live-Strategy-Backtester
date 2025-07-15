#include "analytics/forecasting/PerformanceForecasting.h"

namespace forecasting {

PerformanceForecaster::PerformanceForecaster() {}

void PerformanceForecaster::train(const std::vector<sample_type>& samples, const std::vector<double>& labels) {
    trainer_type trainer;
    trainer.set_c(10);
    df_ = trainer.train(samples, labels);
}

double PerformanceForecaster::predict(const sample_type& sample) {
    return df_(sample);
}

} // namespace forecasting
