#pragma once
#include "mt/state/spectral_state.hpp"

namespace mt {

struct IntersticeDynamics {
    Vec<real> theta;       // θ(s)
    Vec<real> kappa;       // κ(s) = θ'(s) on the parameter axis
    Vec<real> delta;       // δ(s) = exp(u θ(s))   (real part on real-axis run)
    Vec<real> gamma_path;  // γ(s) = ∫ δ(σ) dσ
    Vec<real> s_grid;
};

struct IntersticeTrajectory {
    Vec<cplx> T;     // T_t^{int}(s,h)  (s_index x H)
    Vec<cplx> L;     // Π_3 T_t^{int}(s,h)
    Vec<real> P;     // exp(Re L)
    Vec<cplx> R;     // L(s,h) - L(s,0)
    Vec<real> Risk;  // optional energy / variance proxy per (s,h)
    Vec<cplx> T_grid; // flattened [s_count][H], row-major in s then h
    Vec<real> s_grid;
    Vec<real> h_grid;
    Vec<real> kappa;
    real ds    = 0.0;
    int s_count = 0;
    int H       = 0;
};

IntersticeDynamics build_interstice_dynamics(int s_count, real ds, real u_imag);

Vec<cplx> eval_interstice_field(const SpectralDecomposition& sd,
                                const IntersticeDynamics&    dyn,
                                int s_index,
                                const HorizonGrid&           hg);

IntersticeTrajectory build_interstice_trajectory(const SpectralDecomposition& sd,
                                                 const IntersticeDynamics&    dyn,
                                                 int s_index,
                                                 const HorizonGrid&           hg);

}  // namespace mt
