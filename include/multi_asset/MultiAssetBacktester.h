#ifndef MULTI_ASSET_BACKTESTER_H
#define MULTI_ASSET_BACKTESTER_H

#include "core/Backtester.h"
#include <map>
#include <string>

namespace multi_asset {

class MultiAssetBacktester {
public:
    void add_backtester(const std::string& asset_id, Backtester backtester);
    void run_backtests();
    void optimize_portfolio();

private:
    std::map<std::string, Backtester> backtesters_;
};

} // namespace multi_asset

#endif // MULTI_ASSET_BACKTESTER_H
