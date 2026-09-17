#include "mt/cuda/kernels_hankel.cuh"
namespace mt {
void launch_build_hankel(const thrust::complex<double>*,
						 int,
						 int r,
						 thrust::complex<double>* H_d,
						 cudaStream_t s)
{
	if (r > 0 && H_d)
		cudaMemsetAsync(H_d, 0, sizeof(thrust::complex<double>) * static_cast<size_t>(r) * static_cast<size_t>(r), s);
}

void launch_hankel_minors(const thrust::complex<double>*,
						  int r,
						  thrust::complex<double>* minors_d,
						  cudaStream_t s)
{
	if (r > 0 && minors_d)
		cudaMemsetAsync(minors_d, 0, sizeof(thrust::complex<double>) * static_cast<size_t>(r), s);
}
}
