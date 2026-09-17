#pragma once
#include <cstddef>
#include <cuda_runtime.h>
#include <thrust/complex.h>
namespace mt {
void launch_build_delta_gamma(const double* kappa_d,
							  int s_count,
							  double ds,
							  double u_imag,
							  double* delta_d,
							  double* gamma_d,
							  cudaStream_t s);

void launch_eval_interstice_modes(const thrust::complex<double>* lambda_d,
								  const thrust::complex<double>* gamma_mode_d,
								  const int* mu_d,
								  int mode_count,
								  const double* delta_d,
								  int s_count,
								  const double* h_d,
								  int H,
								  thrust::complex<double>* out_d,
								  cudaStream_t s);

void launch_project_price_return(const thrust::complex<double>* T_d,
								 int n,
								 double* L_d,
								 double* P_d,
								 double* R_d,
								 cudaStream_t s);
}
