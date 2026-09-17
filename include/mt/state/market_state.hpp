#pragma once
#include "mt/core/types.hpp"
#include <cmath>

namespace mt {

// Host SoA state: x = (x0,x1,x2,x3,x4) over n bars.
struct MarketStateColumns {
    Vec<real> x0, x1, x2, x3, x4;
    usize     n = 0;

    void resize(usize new_n) {
        x0.resize(new_n); x1.resize(new_n); x2.resize(new_n);
        x3.resize(new_n); x4.resize(new_n);
        n = new_n;
    }
};

struct DeviceMarketStateColumns {
    real* x0_d = nullptr;
    real* x1_d = nullptr;
    real* x2_d = nullptr;
    real* x3_d = nullptr;
    real* x4_d = nullptr;
    usize n    = 0;
};

// Projected readouts.
inline real Pi3(real x0, real x1, real x2, real x3, real x4) { (void)x0;(void)x1;(void)x2;(void)x4; return x3; }
inline real price(real x3) { return std::exp(x3); }

}  // namespace mt
