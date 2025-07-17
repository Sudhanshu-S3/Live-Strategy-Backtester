#ifndef REGIME_DETECTION_H
#define REGIME_DETECTION_H

#include <vector>
#include "data/DataTypes.h"

namespace regime {

enum class MarketRegime {
    BULL,
    BEAR,
    NEUTRAL
};

class RegimeDetector {
public:
    RegimeDetector(int short_window, int long_window);
    MarketRegime detect(const std::vector<Bar>& data);

private:
    int short_window_;
    int long_window_;
};

} // namespace regime

#endif // REGIME_DETECTION_H
