#pragma once
#include "mt/core/types.hpp"
#include "mt/math/finite_differences.hpp"

namespace mt {

// β_k(h) = (1/k!) Π_{j=0}^{k-1} (h + j).
real beta_k(int k, real h);

// Σ_{k=0}^{K-1} (∂^k F(n,κ)/k!) h^k  =  Σ_{k=0}^{K-1} β_k(h) ∇^k x_n
cplx eval_newton_gregory_point(const Vec<cplx>& nabla, real h);

// Convenience: build ∇^k once, evaluate over a horizon grid.
Vec<cplx> eval_newton_gregory_grid(const Vec<cplx>& x, int n, int K, const Vec<real>& h_grid);

// Evaluate full Newton-Gregory series of u (anchored at index 0) over a horizon grid,
// using K = u.size() backward differences.
Vec<cplx> eval_newton_gregory(const Vec<cplx>& u, const HorizonGrid& hg);

}  // namespace mt
