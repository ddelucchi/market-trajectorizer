#pragma once
#include <cstddef>
#include <cuda_runtime.h>
#include <thrust/complex.h>

namespace mt {
void launch_backward_differences(const thrust::complex<double>* u_d,
								 int n,
								 int k_max,
								 thrust::complex<double>* nabla_d,
								 cudaStream_t s);

void launch_eval_newton_grid(const thrust::complex<double>* nabla_d,
							 int k_count,
							 const double* h_d,
							 int h_count,
							 thrust::complex<double>* out_d,
							 cudaStream_t s);
}
