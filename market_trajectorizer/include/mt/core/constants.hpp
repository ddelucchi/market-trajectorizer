#pragma once
#include "mt/core/types.hpp"

namespace mt::constants {

inline constexpr real PI         = 3.14159265358979323846;
inline constexpr real TWO_PI     = 2.0 * PI;
inline constexpr real LOG_VOL_EPS = 1.0;        // x4 := log(volume + LOG_VOL_EPS)
inline constexpr real FP64_TOL   = 1e-10;
inline constexpr real EVOLUTION_EQUALITY_TOL = 1e-9;  // E = J = N = M tolerance

inline constexpr int  DEFAULT_Q          = 5;   // 5-channel base state
inline constexpr int  DEFAULT_ORDER_MAX  = 8;
inline constexpr int  DEFAULT_N_PHI      = 16;
inline constexpr int  DEFAULT_N_PI       = 8;
inline constexpr int  DEFAULT_L_MAX      = 8;
inline constexpr int  DEFAULT_TORUS_M    = 16;  // samples per torus dim
inline constexpr real DEFAULT_RHO_M0     = 0.05;

}  // namespace mt::constants
