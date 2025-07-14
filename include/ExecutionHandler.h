
#ifndef EXECUTION_HANDLER_H
#define EXECUTION_HANDLER_H

#include "DataTypes.h"
#include <optional>

using namespace std;


class ExecutionHandler {
public:
    virtual ~ExecutionHandler() = default;

    
    virtual FillEvent executeOrder(const OrderEvent& order, const Bar& bar) = 0;
};

#endif 