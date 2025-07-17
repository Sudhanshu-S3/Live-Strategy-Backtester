#include "risk_management/DynamicPositionSizer.h"
#include <stdexcept>

namespace risk_management {

DynamicPositionSizer::DynamicPositionSizer(double risk_per_trade)
    : risk_per_trade_(risk_per_trade) {
    if (risk_per_trade <= 0 || risk_per_trade >= 1) {
        throw std::invalid_argument("Risk per trade must be between 0 and 1.");
    }
}

long DynamicPositionSizer::calculate_position_size(double entry_price, double stop_loss_price, double portfolio_value) {
    double risk_amount = portfolio_value * risk_per_trade_;
    double risk_per_share = entry_price - stop_loss_price;
    if (risk_per_share <= 0) {
        return 0; // Avoid division by zero or negative risk
    }
    return static_cast<long>(risk_amount / risk_per_share);
}

} // namespace risk_management
