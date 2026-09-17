#pragma once
#include <cuda_runtime.h>

namespace mt {

inline dim3 grid_for(std::size_t n, int block) {
    return dim3(static_cast<unsigned>((n + block - 1) / block));
}

inline constexpr int DEFAULT_BLOCK = 256;

inline dim3 grid_for(std::size_t n) { return grid_for(n, DEFAULT_BLOCK); }

}  // namespace mt
