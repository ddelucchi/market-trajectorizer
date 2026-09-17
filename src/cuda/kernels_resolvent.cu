#include "mt/cuda/kernels_resolvent.cuh"
namespace mt {
void launch_eval_resolvent_grid(const thrust::complex<double>*,
								int,
								const thrust::complex<double>*,
								int,
								const thrust::complex<double>*,
								int n,
								thrust::complex<double>* out_d,
								cudaStream_t s)
{
	if (n > 0 && out_d)
		cudaMemsetAsync(out_d, 0, sizeof(thrust::complex<double>) * static_cast<size_t>(n), s);
}

void launch_eval_spectral_modes(const thrust::complex<double>*,
								const thrust::complex<double>*,
								const int*,
								int,
								const double*,
								int H,
								thrust::complex<double>* out_d,
								cudaStream_t s)
{
	if (H > 0 && out_d)
		cudaMemsetAsync(out_d, 0, sizeof(thrust::complex<double>) * static_cast<size_t>(H), s);
}
}
