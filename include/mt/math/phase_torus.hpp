#pragma once
#include "mt/state/extracted_coeffs.hpp"
#include "mt/math/quadrature.hpp"
#include <functional>

using cudaStream_t = struct CUstream_st*;

namespace mt {

// F: C^Q -> C, evaluated as a host callback (CPU path).
using FieldCpu = std::function<cplx(const Vec<real>& z_plus_perturb_real,
                                    const Vec<real>& z_plus_perturb_imag)>;

// Per-axis 1D analytic germ G_q : C -> C used by the separable multichannel
// extractor.  Each germ is evaluated independently on its axis quadrature ring.
using FieldCpu1D = std::function<cplx(real z_real, real z_imag)>;

ExtractedCoefficients extract_phase_torus_cpu(
    const FieldCpu& F,
    const Vec<real>& z_real,         // length Q
    const Vec<real>& z_imag,         // length Q
    real rho_m,
    int  Q,
    int  order_max,
    int  M_per_dim);

// Multichannel separable extractor.  The full Q-dimensional analytic germ is
//
//      F(z_0,...,z_{Q-1})  =  Σ_{q=0..Q-1}  G_q(z_q)
//
// so all mixed multi-indices α with |supp α| > 1 vanish identically.  Diagonal
// multi-indices α = α_q · e_q recover the per-axis 1D Cauchy moments
//
//      A_α  =  (1/2π) ∫_0^{2π} G_q(z_q + ρ e^{iψ}) e^{-i α_q ψ} dψ.
//
// This is mathematically equivalent to extract_phase_torus_cpu on the same
// separable F and is the production extractor for the canonical OHLCV state
// (Q = 5 channels x_0..x_4).  Cost is O(Q · (order_max+1) · M_per_dim) instead
// of O((order_max+1)^Q · M_per_dim^Q), making the full-state extraction
// tractable for the trajectorizer.
ExtractedCoefficients extract_phase_torus_separable_cpu(
    const Vec<FieldCpu1D>& G,        // length Q (per-channel germs)
    const Vec<real>&       z_real,   // length Q
    const Vec<real>&       z_imag,   // length Q
    real rho_m,
    int  order_max,
    int  M_per_dim);

// Device-side view; populated by kernels_phase_torus.cu.
struct DeviceFieldView { /* opaque, kernel-provided */ };
struct DeviceVector    { real* data = nullptr; mt::usize n = 0; };

void extract_phase_torus_gpu(
    const DeviceFieldView& F,
    const DeviceVector&    z_d,
    real rho_m,
    int  Q,
    int  order_max,
    int  M_per_dim,
    DeviceExtractedCoefficients& out,
    cudaStream_t s);

}  // namespace mt
