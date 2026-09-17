#pragma once
#include "mt/core/types.hpp"

namespace mt {

struct RecurrenceCoefficients {
    int       r = 0;
    Vec<cplx> c;     // c0..c_{r-1}, monic q_r(λ)=λ^r + c_{r-1}λ^{r-1} + ... + c_0
};

struct CompanionState {
    int       r = 0;
    Vec<cplx> C;     // r x r row-major
    Vec<cplx> Y0;    // initial state (size r)
};

struct RecurrenceState {
    RecurrenceCoefficients rec;
    CompanionState         comp;
};

}  // namespace mt
