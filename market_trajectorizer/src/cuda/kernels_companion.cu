#include "mt/cuda/kernels_companion.cuh"
namespace mt {
void launch_build_companion(const thrust::complex<double>*,
							int r,
							thrust::complex<double>* C_d,
							cudaStream_t s)
{
	if (r > 0 && C_d)
		cudaMemsetAsync(C_d, 0, sizeof(thrust::complex<double>) * static_cast<size_t>(r) * static_cast<size_t>(r), s);
}

void launch_eval_companion_powers(const thrust::complex<double>*,
								  const thrust::complex<double>*,
								  int r,
								  int H,
								  thrust::complex<double>* y_out_d,
								  cudaStream_t s)
{
	if (r > 0 && H > 0 && y_out_d)
		cudaMemsetAsync(y_out_d, 0, sizeof(thrust::complex<double>) * static_cast<size_t>(r) * static_cast<size_t>(H), s);
}
}
