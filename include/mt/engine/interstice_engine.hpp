#pragma once
#include "mt/math/interstice.hpp"
#include "mt/state/spectral_state.hpp"

namespace mt {

IntersticeTrajectory run_interstice(const SpectralDecomposition& sd,
                                    int s_count, real ds, real u_imag,
                                    int s_index_anchor,
                                    const HorizonGrid& hg);

}  // namespace mt
