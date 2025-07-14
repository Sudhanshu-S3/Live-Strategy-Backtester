#include "SimulatedExecutionHandler.h"
using namespace std;

SimulatedExecutionHandler::SimulatedExecutionHandler(double commission_rate)
    : commission_rate(commission_rate) {}

// Simulates the execution of an order.
FillEvent SimulatedExecutionHandler::executeOrder(const OrderEvent& order, const Bar& bar) {
    double fill_price = bar.close;

    
    double commission = fill_price * order.quantity * this->commission_rate;


    FillEvent fill_event = {
        bar.timestamp,     
        order.symbol,
        order.direction,
        order.quantity,
        fill_price,
        commission
    };

    return fill_event;
}
