#ifndef DYNAMIC_POSITION_SIZER_H
#define DYNAMIC_POSITION_SIZER_H

#include "core/Portfolio.h"

namespace risk_management {

class DynamicPositionSizer {
public:
    DynamicPositionSizer(double risk_per_trade);
    long calculate_position_size(double entry_price, double stop_loss_price, double portfolio_value);

private:
    double risk_per_trade_;
};

} // namespace risk_management

#endif // DYNAMIC_POSITION_SIZER_H
