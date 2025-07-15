#ifndef STRESS_TESTER_H
#define STRESS_TESTER_H

#include "core/Backtester.h"
#include <vector>

namespace risk_management {

enum class ScenarioType {
    HISTORICAL_CRASH,
    INTEREST_RATE_HIKE,
    VOLATILITY_SHOCK
};

class StressTester {
public:
    StressTester(Backtester& backtester);
    void run_scenario(ScenarioType scenario);

private:
    Backtester& backtester_;
    void apply_market_shock(double shock_factor);
};

} // namespace risk_management

#endif // STRESS_TESTER_H
