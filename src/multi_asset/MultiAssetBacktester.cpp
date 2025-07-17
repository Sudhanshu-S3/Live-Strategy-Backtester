#include "multi_asset/MultiAssetBacktester.h"

namespace multi_asset {

void MultiAssetBacktester::add_backtester(const std::string& asset_id, std::shared_ptr<Backtester> backtester) {
    backtesters_[asset_id] = backtester;
}

void MultiAssetBacktester::run_backtests() {
    for (auto& pair : backtesters_) {
        pair.second.run();
    }
}

void MultiAssetBacktester::optimize_portfolio() {
    // Placeholder for multi-asset portfolio optimization logic
}

} // namespace multi_asset
