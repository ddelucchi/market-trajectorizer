#pragma once
#include "mt/core/types.hpp"

namespace mt {

// Index-set N defining H_t^{(N)} = σ(X_t, X_{t-1}, ..., X_{t-N}).
struct HistoryFiltration {
    int N = -1;        // -1 means H_t^{(∞)}
};

}  // namespace mt
