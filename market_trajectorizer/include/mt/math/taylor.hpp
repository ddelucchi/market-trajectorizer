#pragma once
#include "mt/state/jet_state.hpp"

namespace mt {

// D_i = ε_m^{-1} ∂_{η_i}, I_i = ε_m I_i Π_{η_i,⊥}.
// Invariants: D_i I_i = Π_{η_i,⊥},  I_i D_i = Π_{η_i,⊥}.
JetState apply_D(const JetState& J, int i, real eps_m);
JetState apply_I(const JetState& J, int i, real eps_m);
JetState project_perp(const JetState& J, int i);

}  // namespace mt
