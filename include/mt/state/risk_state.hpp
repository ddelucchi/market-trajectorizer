#pragma once
#include "mt/core/types.hpp"
#include <string>

namespace mt {

struct RiskField {
    Vec<real> energy_framework;
    Vec<real> energy_proxy;
    Vec<real> curvature;
    Vec<real> drawdown_proxy;
    Vec<real> uncertainty_proxy;
    // Provenance: framework energy is theorem-side carrier (tr.Risk).
    // If false, energy_framework is empty (NOT silently filled with proxy).
    // Production code MUST gate on this flag.
    bool        energy_framework_present     = false;
    // Machine-readable reason populated when energy_framework_present=false.
    // Carried into signal.json / diagnostics.json / CLI stderr so production
    // failures cite the precise cause.
    std::string energy_framework_missing_reason;
};

}  // namespace mt
