#include "mt/math/combinatorics.hpp"
#include "mt/core/errors.hpp"
#include <numeric>

namespace mt {

real factorial_int(int n) {
    if (n < 0) throw NumericError("factorial of negative");
    real r = 1.0;
    for (int k = 2; k <= n; ++k) r *= static_cast<real>(k);
    return r;
}

real multi_factorial(const Vec<int>& alpha) {
    real r = 1.0;
    for (int a : alpha) r *= factorial_int(a);
    return r;
}

int multi_degree(const Vec<int>& alpha) {
    return std::accumulate(alpha.begin(), alpha.end(), 0);
}

usize multi_index_count(int Q, int order_max) {
    // Σ_{d=0}^{order_max} C(Q + d - 1, d). Compute via Pascal recursion.
    Vec<Vec<real>> C(Q + order_max + 1, Vec<real>(Q + order_max + 1, 0.0));
    for (int n = 0; n <= Q + order_max; ++n) {
        C[n][0] = 1.0;
        for (int k = 1; k <= n; ++k) C[n][k] = C[n-1][k-1] + C[n-1][k];
    }
    real total = 0.0;
    for (int d = 0; d <= order_max; ++d) {
        total += C[Q + d - 1][d];
    }
    return static_cast<usize>(total);
}

static void enumerate_recursive(int Q, int remaining, Vec<int>& cur, Vec<Vec<int>>& out) {
    if (static_cast<int>(cur.size()) == Q) { out.push_back(cur); return; }
    for (int v = 0; v <= remaining; ++v) {
        cur.push_back(v);
        enumerate_recursive(Q, remaining - v, cur, out);
        cur.pop_back();
    }
}

void enumerate_multi_indices(int Q, int order_max, Vec<Vec<int>>& out) {
    out.clear();
    Vec<int> cur; cur.reserve(static_cast<usize>(Q));
    enumerate_recursive(Q, order_max, cur, out);
}

MultiIndexLayout make_multi_index_layout(int Q, int order_max) {
    MultiIndexLayout L;
    L.Q = Q; L.order_max = order_max;
    L.strides.resize(static_cast<usize>(Q));
    // Stride 1 along last axis; deterministic dense (order_max+1)^Q layout.
    usize stride = 1;
    for (int i = Q - 1; i >= 0; --i) {
        L.strides[static_cast<usize>(i)] = stride;
        stride *= static_cast<usize>(order_max + 1);
    }
    return L;
}

usize flatten_multi_index(const Vec<int>& alpha, const MultiIndexLayout& L) {
    usize idx = 0;
    for (int i = 0; i < L.Q; ++i) idx += static_cast<usize>(alpha[static_cast<usize>(i)]) * L.strides[static_cast<usize>(i)];
    return idx;
}

real binom(int n, int k) {
    if (k < 0 || k > n) return 0.0;
    real r = 1.0;
    for (int j = 1; j <= k; ++j) r = r * static_cast<real>(n - j + 1) / static_cast<real>(j);
    return r;
}

real binom_real(real h, int s) {
    if (s < 0) return 0.0;
    real r = 1.0;
    for (int j = 0; j < s; ++j) r *= (h - static_cast<real>(j));
    return r / factorial_int(s);
}

}  // namespace mt
