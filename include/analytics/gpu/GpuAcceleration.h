#ifndef GPU_ACCELERATION_H
#define GPU_ACCELERATION_H

#include <vector>

namespace gpu {

void parallel_sum(const std::vector<float>& a, const std::vector<float>& b, std::vector<float>& c);

} // namespace gpu

#endif // GPU_ACCELERATION_H
