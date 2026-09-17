#include "mt/engine/interstice_engine.hpp"

namespace mt {

IntersticeTrajectory run_interstice(const SpectralDecomposition& sd,
                                    int s_count, real ds, real u_imag,
                                    int s_index_anchor,
                                    const HorizonGrid& hg)
{
    auto dyn = build_interstice_dynamics(s_count, ds, u_imag);
    return build_interstice_trajectory(sd, dyn, s_index_anchor, hg);
}

}  // namespace mt
