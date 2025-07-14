#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include <optional>
#include "DataTypes.h"

using namespace std;
// Abstract base class for handling market data (live or historical).
class DataHandler {
public:
    // Virtual destructor is essential for base classes.
    virtual ~DataHandler() = default;

    // Pure virtual function to get the next available bar.
    // Returns an empty optional if there is no more data.
    virtual optional<Bar> getLatestBar() = 0;
};

#endif