#pragma once
#include "mt/core/types.hpp"

namespace mt {

inline real apply_slippage(real px, int side, real slippage_bps) {
    return px * (1.0 + side * slippage_bps * 1.0e-4);
}

}  // namespace mt
