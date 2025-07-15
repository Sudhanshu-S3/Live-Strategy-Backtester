#include "GpuAcceleration.h"
#include <cuda_runtime.h>
#include <iostream>

__global__ void vector_add(const float* a, const float* b, float* c, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        c[i] = a[i] + b[i];
    }
}

namespace gpu {

void parallel_sum(const std::vector<float>& a, const std::vector<float>& b, std::vector<float>& c) {
    if (a.size() != b.size()) {
        throw std::invalid_argument("Vectors must have the same size");
    }
    c.resize(a.size());

    float *d_a, *d_b, *d_c;
    size_t size = a.size() * sizeof(float);

    cudaMalloc(&d_a, size);
    cudaMalloc(&d_b, size);
    cudaMalloc(&d_c, size);

    cudaMemcpy(d_a, a.data(), size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, b.data(), size, cudaMemcpyHostToDevice);

    int threadsPerBlock = 256;
    int blocksPerGrid = (a.size() + threadsPerBlock - 1) / threadsPerBlock;

    vector_add<<<blocksPerGrid, threadsPerBlock>>>(d_a, d_b, d_c, a.size());

    cudaMemcpy(c.data(), d_c, size, cudaMemcpyDeviceToHost);

    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_c);
}

} // namespace gpu
