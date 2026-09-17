#pragma once
#include "mt/state/recurrence_state.hpp"
#include "mt/state/spectral_state.hpp"

namespace mt {

struct RecurrencePipelineOut {
    RecurrenceCoefficients rec;
    CompanionState         comp;
    RationalModel          rational;
    SpectralDecomposition  spectrum;
};

RecurrencePipelineOut build_recurrence_pipeline(const Vec<cplx>& u,
                                                int  N_max,
                                                real tol);

}  // namespace mt
