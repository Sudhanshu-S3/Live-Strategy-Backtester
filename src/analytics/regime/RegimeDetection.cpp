#include "regime/RegimeDetection.h"
#include <numeric>
#include <stdexcept>

namespace regime {

RegimeDetector::RegimeDetector(int short_window, int long_window)
    : short_window_(short_window), long_window_(long_window) {}

MarketRegime RegimeDetector::detect(const std::vector<Bar>& data) {
    if (data.size() < long_window_) {
        return MarketRegime::NEUTRAL;
    }

    double short_sma = 0.0;
    double long_sma = 0.0;

    for (size_t i = data.size() - short_window_; i < data.size(); ++i) {
        short_sma += data[i].close;
    }
    short_sma /= short_window_;

    for (size_t i = data.size() - long_window_; i < data.size(); ++i) {
        long_sma += data[i].close;
    }
    long_sma /= long_window_;

    if (short_sma > long_sma * 1.01) {
        return MarketRegime::BULL;
    } else if (short_sma < long_sma * 0.99) {
        return MarketRegime::BEAR;
    } else {
        return MarketRegime::NEUTRAL;
    }
}

} // namespace regime
