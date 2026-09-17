#pragma once
#include "mt/core/config.hpp"
#include "mt/data/feature_map.hpp"
#include "mt/state/extracted_coeffs.hpp"
#include "mt/state/interstice_state.hpp"
#include "mt/state/jet_state.hpp"

namespace mt {

ChangePointSet phase_1_sectorizer_cpu(const MarketStateColumns& s, const ChangePointConfig& cfg);

ChangePointSet detect_changepoints_cpu(const MarketStateColumns& s, const ChangePointConfig& cfg);

PiecewiseSection build_piecewise_section_cpu(const MarketStateColumns& s,
                                             int                        t_anchor,
                                             const ChangePointSet&      cps,
                                             const SectionConfig&       cfg,
                                             const ChangePointConfig&   cp_cfg);

FutureSection build_past_jet_state_cpu(const PiecewiseSection& sec,
                                       const JetState&         normalized_jet,
                                       const MarketStateColumns& market_state,
                                       int                     t_anchor,
                                       int                     N_phi,
                                       int                     N_pi,
                                       int                     L_max);

// Strict ledger clause 13.1 / 13.6:
//   The theorem-equality carriers (continuous / jet / Newton / matrix) MUST be
//   driven from the SMOOTH analytic continuation of the past jet (A_phi only).
//   The packet/discontinuity contribution (A_pi, sigma) belongs to the
//   trajectory / interstice / price-readout layer.  Mixing them silently
//   contaminates the four-way identity by construction and is forbidden.
//
//   `evaluate_smooth_future_section` evaluates the smooth A_phi polynomial
//     only.  Use this everywhere the four-way carrier identity is exercised
//     (`compute_evolution_equality`, `max_carrier_residual`, `u_carrier`,
//     `u_past_smooth`).
//
//   `evaluate_full_future_section` evaluates A_phi + activated A_pi packets.
//     Use this in the trajectory / readout / risk layer where packet
//     contributions are intended.
//
//   `evaluate_future_section` is preserved as a back-compat alias for
//     `evaluate_full_future_section`.  New theorem-path code MUST NOT call
//     this name; new code MUST pick smooth or full explicitly.
cplx evaluate_smooth_future_section(const FutureSection& Jminus, real h);
cplx evaluate_full_future_section  (const FutureSection& Jminus, real h);
[[deprecated("legacy alias maps to the full packet path; use evaluate_smooth_future_section or evaluate_full_future_section explicitly")]]
cplx evaluate_future_section       (const FutureSection& Jminus, real h);  // = full

}  // namespace mt
