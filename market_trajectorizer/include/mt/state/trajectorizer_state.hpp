#pragma once
#include "mt/state/extracted_coeffs.hpp"
#include "mt/state/interstice_state.hpp"
#include "mt/state/jet_state.hpp"
#include "mt/state/recurrence_state.hpp"
#include "mt/state/spectral_state.hpp"

namespace mt {

struct EngineState {
    int                  anchor_idx = 0;
    FutureSection        Jminus;             // J_t^-
    JetState             jet;                // J_α[F](z, κ) at anchor
    ExtractedCoefficients extracted;
    RecurrenceState      rec;
    SpectralState        spec;
};

}  // namespace mt
