#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include <optional>
#include "DataTypes.h"
using namespace std;
class DataHandler {
public:
    virtual ~DataHandler() = default;
    
    virtual optional<pair<string, Bar>> getLatestBar() = 0;
};

#endif