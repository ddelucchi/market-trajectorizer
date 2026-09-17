#pragma once
#include <cstddef>
#include <cuda_runtime.h>
#include <thrust/complex.h>
namespace mt {
void launch_eval_energy(const thrust::complex<double>* T_d,
						int n,
						double* energy_d,
						cudaStream_t s);

void launch_eval_drawdown_proxy(const double* energy_d,
								int n,
								double* drawdown_d,
								cudaStream_t s);
}
