#pragma once
#include <cstddef>
#include <cuda_runtime.h>
#include <thrust/complex.h>
namespace mt {
void launch_build_companion(const thrust::complex<double>* c_d,
							int r,
							thrust::complex<double>* C_d,
							cudaStream_t s);

void launch_eval_companion_powers(const thrust::complex<double>* C_d,
								  const thrust::complex<double>* y0_d,
								  int r,
								  int H,
								  thrust::complex<double>* y_out_d,
								  cudaStream_t s);
}
