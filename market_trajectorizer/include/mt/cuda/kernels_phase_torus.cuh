#pragma once
#include <cstddef>
#include <cuda_runtime.h>
#include <thrust/complex.h>

namespace mt {

void launch_eval_phase_samples(const double* z_re_d,
							   const double* z_im_d,
							   const double* psi_d,
							   int Q,
							   int M_per_dim,
							   int order_count,
							   double rho_m,
							   thrust::complex<double>* F_samples_d,
							   cudaStream_t s);

void launch_reduce_torus_coeffs(const thrust::complex<double>* F_samples_d,
								const int* alpha_d,
								const double* psi_d,
								int Q,
								int nodes,
								int order_count,
								thrust::complex<double>* A_d,
								cudaStream_t s);

}  // namespace mt
