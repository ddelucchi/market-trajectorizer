#include "mt/cuda/kernels_newton.cuh"
namespace mt {
void launch_backward_differences(const thrust::complex<double>*,
								 int,
								 int k_max,
								 thrust::complex<double>* nabla_d,
								 cudaStream_t s)
{
	if (k_max > 0 && nabla_d)
		cudaMemsetAsync(nabla_d, 0, sizeof(thrust::complex<double>) * static_cast<size_t>(k_max), s);
}

void launch_eval_newton_grid(const thrust::complex<double>*,
							 int,
							 const double*,
							 int h_count,
							 thrust::complex<double>* out_d,
							 cudaStream_t s)
{
	if (h_count > 0 && out_d)
		cudaMemsetAsync(out_d, 0, sizeof(thrust::complex<double>) * static_cast<size_t>(h_count), s);
}
}
