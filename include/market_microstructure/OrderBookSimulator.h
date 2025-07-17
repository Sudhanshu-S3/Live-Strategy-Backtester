#ifndef ORDER_BOOK_SIMULATOR_H
#define ORDER_BOOK_SIMULATOR_H

#include "../data/DataTypes.h"
#include <map>
#include <vector>

namespace market_microstructure {

class OrderBookSimulator {
public:
    void add_order(const Order& order);
    void cancel_order(long order_id);
    void match_orders();
    void update_liquidity(const Bar& new_bar);

    const std::map<double, long, std::greater<double>>& get_bids() const;
    const std::map<double, long>& get_asks() const;

private:
    std::map<double, long, std::greater<double>> bids_; // price -> quantity
    std::map<double, long> asks_; // price -> quantity
};

} // namespace market_microstructure

#endif // ORDER_BOOK_SIMULATOR_H
