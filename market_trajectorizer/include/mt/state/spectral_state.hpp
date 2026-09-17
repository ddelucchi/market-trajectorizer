#pragma once
#include "mt/core/types.hpp"

namespace mt {

struct SpectralMode {
    cplx      lambda;
    int       multiplicity = 1;     // μ_j
    Vec<cplx> gamma;                // γ_{j,s}, s = 0..μ_j-1
};

struct SpectralDecomposition {
    int               r = 0;
    Vec<SpectralMode> modes;
};

struct RationalModel {
    Vec<cplx> P;      // numerator P_{r-1}(z)
    Vec<cplx> Q;      // denominator Q_r(z), Q[0] = 1
};

struct SpectralState {
    RationalModel         rational;
    SpectralDecomposition spectrum;
};

}  // namespace mt
