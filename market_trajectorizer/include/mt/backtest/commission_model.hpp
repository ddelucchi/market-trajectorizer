#pragma once
#include "mt/core/types.hpp"

namespace mt {

inline real commission(real notional, real fee_bps) {
    return notional * fee_bps * 1.0e-4;
}

}  // namespace mt
