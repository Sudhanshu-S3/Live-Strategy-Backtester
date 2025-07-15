#include "analytics/gpu/GpuAcceleration.h"
#include <cuda_runtime.h>
#include <stdexcept>
#include <iostream>

// CUDA Kernel for element-wise vector addition
__global__ void add_vectors_kernel(const float* a, const float* b, float* c, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        c[idx] = a[idx] + b[idx];
    }
}

namespace gpu {

void parallel_sum(const std::vector<float>& a, const std::vector<float>& b, std::vector<float>& c) {
    if (a.size() != b.size()) {
        throw std::invalid_argument("Input vectors must have the same size.");
    }
    if (a.empty()) {
        c.clear();
        return;
    }

    const int n = a.size();
    c.resize(n);

    float *d_a, *d_b, *d_c;
    size_t size = n * sizeof(float);

    // Allocate memory on the GPU
    cudaError_t err = cudaMalloc((void**)&d_a, size);
    if (err != cudaSuccess) throw std::runtime_error("Failed to allocate device memory for a.");
    
    err = cudaMalloc((void**)&d_b, size);
    if (err != cudaSuccess) {
        cudaFree(d_a);
        throw std::runtime_error("Failed to allocate device memory for b.");
    }

    err = cudaMalloc((void**)&d_c, size);
    if (err != cudaSuccess) {
        cudaFree(d_a);
        cudaFree(d_b);
        throw std::runtime_error("Failed to allocate device memory for c.");
    }

    // Copy data from host to device
    err = cudaMemcpy(d_a, a.data(), size, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        cudaFree(d_a); cudaFree(d_b); cudaFree(d_c);
        throw std::runtime_error("Failed to copy a to device.");
    }

    err = cudaMemcpy(d_b, b.data(), size, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        cudaFree(d_a); cudaFree(d_b); cudaFree(d_c);
        throw std::runtime_error("Failed to copy b to device.");
    }

    // Launch the kernel
    int threadsPerBlock = 256;
    int blocksPerGrid = (n + threadsPerBlock - 1) / threadsPerBlock;
    add_vectors_kernel<<<blocksPerGrid, threadsPerBlock>>>(d_a, d_b, d_c, n);
    
    // Check for kernel launch errors
    err = cudaGetLastError();
    if (err != cudaSuccess) {
        cudaFree(d_a); cudaFree(d_b); cudaFree(d_c);
        throw std::runtime_error("CUDA kernel launch failed: " + std::string(cudaGetErrorString(err)));
    }

    // Copy result from device to host
    err = cudaMemcpy(c.data(), d_c, size, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
        cudaFree(d_a); cudaFree(d_b); cudaFree(d_c);
        throw std::runtime_error("Failed to copy result from device.");
    }

    // Free GPU memory
    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_c);
}

} // namespace gpu
