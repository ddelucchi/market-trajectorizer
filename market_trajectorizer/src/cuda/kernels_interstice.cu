#include "mt/cuda/kernels_interstice.cuh"
namespace mt {
void launch_build_delta_gamma(const double*,
							  int s_count,
							  double,
							  double,
							  double* delta_d,
							  double* gamma_d,
							  cudaStream_t s)
{
	if (s_count > 0 && delta_d)
		cudaMemsetAsync(delta_d, 0, sizeof(double) * static_cast<size_t>(s_count), s);
	if (s_count > 0 && gamma_d)
		cudaMemsetAsync(gamma_d, 0, sizeof(double) * static_cast<size_t>(s_count), s);
}

void launch_eval_interstice_modes(const thrust::complex<double>*,
								  const thrust::complex<double>*,
								  const int*,
								  int,
								  const double*,
								  int s_count,
								  const double*,
								  int H,
								  thrust::complex<double>* out_d,
								  cudaStream_t s)
{
	if (s_count > 0 && H > 0 && out_d)
		cudaMemsetAsync(out_d, 0, sizeof(thrust::complex<double>) * static_cast<size_t>(s_count) * static_cast<size_t>(H), s);
}

void launch_project_price_return(const thrust::complex<double>*,
								 int n,
								 double* L_d,
								 double* P_d,
								 double* R_d,
								 cudaStream_t s)
{
	if (n > 0 && L_d) cudaMemsetAsync(L_d, 0, sizeof(double) * static_cast<size_t>(n), s);
	if (n > 0 && P_d) cudaMemsetAsync(P_d, 0, sizeof(double) * static_cast<size_t>(n), s);
	if (n > 0 && R_d) cudaMemsetAsync(R_d, 0, sizeof(double) * static_cast<size_t>(n), s);
}
}
