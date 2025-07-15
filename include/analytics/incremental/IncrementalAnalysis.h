#ifndef INCREMENTAL_ANALYSIS_H
#define INCREMENTAL_ANALYSIS_H

#include <vector>

class IncrementalMean {
public:
    IncrementalMean();
    void update(double new_value);
    double get_mean() const;

private:
    double mean_;
    int count_;
};

#endif // INCREMENTAL_ANALYSIS_H
