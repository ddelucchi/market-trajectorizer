#include "mt/cuda/kernels_risk.cuh"
namespace mt {
void launch_eval_energy(const thrust::complex<double>*,
						int n,
						double* energy_d,
						cudaStream_t s)
{
	if (n > 0 && energy_d)
		cudaMemsetAsync(energy_d, 0, sizeof(double) * static_cast<size_t>(n), s);
}

void launch_eval_drawdown_proxy(const double*,
								int n,
								double* drawdown_d,
								cudaStream_t s)
{
	if (n > 0 && drawdown_d)
		cudaMemsetAsync(drawdown_d, 0, sizeof(double) * static_cast<size_t>(n), s);
}
}
