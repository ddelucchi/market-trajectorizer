#pragma once
#include "mt/core/errors.hpp"
#include <cuda_runtime.h>
#include <string>

#define MT_CUDA_CHECK(expr)                                                             \
    do {                                                                                \
        cudaError_t _err = (expr);                                                      \
        if (_err != cudaSuccess) {                                                      \
            throw ::mt::CudaError(std::string("CUDA error: ") + cudaGetErrorString(_err)\
                                  + " at " __FILE__ ":" + std::to_string(__LINE__));    \
        }                                                                               \
    } while (0)
