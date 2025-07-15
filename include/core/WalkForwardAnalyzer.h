#ifndef WALK_FORWARD_ANALYZER_H
#define WALK_FORWARD_ANALYZER_H

#include "../lib/nlohmann/json.hpp"

using json = nlohmann::json;

class WalkForwardAnalyzer {
public:
    WalkForwardAnalyzer(const json& config);
    void run();

private:
    json config_;
    json walk_forward_params_;
};

#endif // WALK_FORWARD_ANALYZER_H 