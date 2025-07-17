#include "incremental/IncrementalAnalysis.h"

IncrementalMean::IncrementalMean() : mean_(0.0), count_(0) {}

void IncrementalMean::update(double new_value) {
    count_++;
    mean_ += (new_value - mean_) / count_;
}

double IncrementalMean::get_mean() const {
    return mean_;
}
