#pragma once
#include "mt/core/types.hpp"
// Host complex re-export. Device kernels use thrust::complex / cuda::std::complex
// in their own translation units (see src/cuda/*.cu).
namespace mt {
using complex64 = cplx;
}
