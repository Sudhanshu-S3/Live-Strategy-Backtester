#ifndef CIRCUIT_BREAKER_H
#define CIRCUIT_BREAKER_H

#include "core/Portfolio.h"

namespace risk_management {

class CircuitBreaker {
public:
    CircuitBreaker(double max_drawdown, long time_window);
    bool check_drawdown(const Portfolio& portfolio);
    void reset();

private:
    double max_drawdown_;
    long time_window_;
    // Add state for tracking drawdown over time
};

} // namespace risk_management

#endif // CIRCUIT_BREAKER_H
