#pragma once
#include <cuda_runtime.h>

namespace mt {

void launch_build_state(const double* open, const double* high, const double* low,
                        const double* close, const double* volume,
                        double* x0, double* x1, double* x2, double* x3, double* x4,
                        std::size_t n, cudaStream_t s);

}  // namespace mt
