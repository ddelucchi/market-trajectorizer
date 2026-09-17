#include "mt/cuda/kernels_feature.cuh"
#include "mt/cuda/launch.hpp"
#include <cuda_runtime.h>
#include <math_constants.h>

namespace mt {

__global__ void k_build_state(const double* __restrict__ open,
                              const double* __restrict__ high,
                              const double* __restrict__ low,
                              const double* __restrict__ close,
                              const double* __restrict__ volume,
                              double* __restrict__ x0, double* __restrict__ x1,
                              double* __restrict__ x2, double* __restrict__ x3,
                              double* __restrict__ x4, std::size_t n)
{
    std::size_t i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    x0[i] = log(open[i]);
    x1[i] = log(high[i]);
    x2[i] = log(low[i]);
    x3[i] = log(close[i]);
    x4[i] = log(volume[i] + 1.0);
}

void launch_build_state(const double* open, const double* high, const double* low,
                        const double* close, const double* volume,
                        double* x0, double* x1, double* x2, double* x3, double* x4,
                        std::size_t n, cudaStream_t s)
{
    if (n == 0) return;
    dim3 block(DEFAULT_BLOCK), grid(grid_for(n));
    k_build_state<<<grid, block, 0, s>>>(open, high, low, close, volume,
                                          x0, x1, x2, x3, x4, n);
}

}  // namespace mt
