#pragma once
// Fixed CUDA stream graph topology (determinism contract):
//   stream 0: H2D ingest
//   stream 1: feature construction
//   stream 2: phase-torus extraction
//   stream 3: hankel/recurrence/resolvent
//   stream 4: interstice/risk
//   stream 5: D2H results
namespace mt {

enum class StreamId : int {
    H2D       = 0,
    Feature   = 1,
    Torus     = 2,
    Recurrence= 3,
    Interstice= 4,
    D2H       = 5,
    Count
};

}  // namespace mt
