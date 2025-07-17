#include "market_microstructure/OrderBookSimulator.h"

namespace market_microstructure {

void OrderBookSimulator::add_order(const Order& order) {
    // Simplified logic for adding an order
    if (order.side == OrderSide::BUY) {
        bids_[order.price] += order.quantity;
    } else {
        asks_[order.price] += order.quantity;
    }
}

void OrderBookSimulator::cancel_order(long order_id) {
    // In a real system, you'd need a way to track orders by ID
    // This is a simplified placeholder
}

void OrderBookSimulator::match_orders() {
    // Simplified matching logic
    while (!bids_.empty() && !asks_.empty() && bids_.begin()->first >= asks_.begin()->first) {
        auto& bid = *bids_.begin();
        auto& ask = *asks_.begin();

        long trade_quantity = std::min(bid.second, ask.second);

        bid.second -= trade_quantity;
        ask.second -= trade_quantity;

        if (bid.second == 0) {
            bids_.erase(bids_.begin());
        }
        if (ask.second == 0) {
            asks_.erase(asks_.begin());
        }
    }
}

void OrderBookSimulator::update_liquidity(const Bar& new_bar) {
    // Simplified liquidity update based on a new bar
    // This could involve adding or removing liquidity around the new price
}

const std::map<double, long, std::greater<double>>& OrderBookSimulator::get_bids() const {
    return bids_;
}

const std::map<double, long>& OrderBookSimulator::get_asks() const {
    return asks_;
}

} // namespace market_microstructure
