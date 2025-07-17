#include "serialization/DataSerialization.h"
#include <fstream>
#include <stdexcept>

namespace serialization {

void serialize_bars(const std::vector<Bar>& bars, const std::string& file_path) {
    std::ofstream out(file_path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to open file for writing: " + file_path);
    }
    for (const auto& bar : bars) {
        out.write(reinterpret_cast<const char*>(&bar), sizeof(Bar));
    }
}

std::vector<Bar> deserialize_bars(const std::string& file_path) {
    std::ifstream in(file_path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open file for reading: " + file_path);
    }
    std::vector<Bar> bars;
    Bar bar;
    while (in.read(reinterpret_cast<char*>(&bar), sizeof(Bar))) {
        bars.push_back(bar);
    }
    return bars;
}

} // namespace serialization
