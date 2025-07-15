#include "ml/MLModel.h"

namespace ml {

StrategyClassifier::StrategyClassifier() {}

void StrategyClassifier::train(const std::vector<sample_type>& samples, const std::vector<double>& labels) {
    trainer_type trainer;
    trainer.set_kernel(kernel_type(0.1));
    trainer.set_c(10);
    df_ = trainer.train(samples, labels);
}

double StrategyClassifier::predict(const sample_type& sample) {
    return df_(sample);
}

} // namespace ml
