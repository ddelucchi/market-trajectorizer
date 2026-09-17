#pragma once
#include "mt/core/types.hpp"

namespace mt {

// One OHLCV bar in canonical units (post-validation).
struct Candle {
    Timestamp ts;     // UNIX nanoseconds
    real open;
    real high;
    real low;
    real close;
    real volume;
};

// Column-major (struct-of-arrays) host candle store.
struct CandleColumns {
    Vec<Timestamp> ts;
    Vec<real>      open;
    Vec<real>      high;
    Vec<real>      low;
    Vec<real>      close;
    Vec<real>      volume;
    usize          n = 0;

    void resize(usize new_n) {
        ts.resize(new_n); open.resize(new_n); high.resize(new_n);
        low.resize(new_n); close.resize(new_n); volume.resize(new_n);
        n = new_n;
    }
};

// Mirrored device pointers (raw, non-owning view); ownership lives in DeviceBuffer<T>.
struct DeviceCandleColumns {
    Timestamp* ts_d   = nullptr;
    real*      open_d = nullptr;
    real*      high_d = nullptr;
    real*      low_d  = nullptr;
    real*      close_d= nullptr;
    real*      volume_d = nullptr;
    usize      n      = 0;
};

}  // namespace mt
