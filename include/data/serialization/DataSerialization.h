#ifndef DATA_SERIALIZATION_H
#define DATA_SERIALIZATION_H

#include "data/DataTypes.h"
#include <vector>
#include <string>

namespace serialization {

void serialize_bars(const std::vector<Bar>& bars, const std::string& file_path);
std::vector<Bar> deserialize_bars(const std::string& file_path);

} // namespace serialization

#endif // DATA_SERIALIZATION_H
