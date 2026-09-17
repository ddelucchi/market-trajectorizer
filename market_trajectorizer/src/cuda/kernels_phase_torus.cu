#include "mt/cuda/kernels_phase_torus.cuh"
namespace mt {
void launch_eval_phase_samples(const double*,
							   const double*,
							   const double*,
							   int,
							   int,
							   int order_count,
							   double,
							   thrust::complex<double>* F_samples_d,
							   cudaStream_t s)
{
	if (order_count > 0 && F_samples_d)
		cudaMemsetAsync(F_samples_d, 0, sizeof(thrust::complex<double>) * static_cast<size_t>(order_count), s);
}

void launch_reduce_torus_coeffs(const thrust::complex<double>*,
								const int*,
								const double*,
								int,
								int,
								int order_count,
								thrust::complex<double>* A_d,
								cudaStream_t s)
{
	if (order_count > 0 && A_d)
		cudaMemsetAsync(A_d, 0, sizeof(thrust::complex<double>) * static_cast<size_t>(order_count), s);
}
}
