#pragma once
#include <cstddef>
#include <cuda_runtime.h>
#include <thrust/complex.h>
namespace mt {
void launch_build_hankel(const thrust::complex<double>* u_d,
						 int n,
						 int r,
						 thrust::complex<double>* H_d,
						 cudaStream_t s);

void launch_hankel_minors(const thrust::complex<double>* H_d,
						  int r,
						  thrust::complex<double>* minors_d,
						  cudaStream_t s);
}
