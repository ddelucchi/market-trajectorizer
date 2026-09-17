#pragma once
#include <cstddef>
#include <cuda_runtime.h>
#include <thrust/complex.h>
namespace mt {
void launch_eval_resolvent_grid(const thrust::complex<double>* P_d,
								int p_deg,
								const thrust::complex<double>* Q_d,
								int q_deg,
								const thrust::complex<double>* z_d,
								int n,
								thrust::complex<double>* out_d,
								cudaStream_t s);

void launch_eval_spectral_modes(const thrust::complex<double>* lambda_d,
								const thrust::complex<double>* gamma_d,
								const int* mu_d,
								int mode_count,
								const double* h_d,
								int H,
								thrust::complex<double>* out_d,
								cudaStream_t s);
}
