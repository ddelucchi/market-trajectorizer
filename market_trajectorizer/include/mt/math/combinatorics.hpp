#pragma once
#include "mt/core/types.hpp"

namespace mt {

real factorial_int(int n);
real multi_factorial(const Vec<int>& alpha);
int  multi_degree(const Vec<int>& alpha);

usize multi_index_count(int Q, int order_max);
void  enumerate_multi_indices(int Q, int order_max, Vec<Vec<int>>& out);
usize flatten_multi_index(const Vec<int>& alpha, const MultiIndexLayout& layout);

MultiIndexLayout make_multi_index_layout(int Q, int order_max);

real binom(int n, int k);                       // (n choose k), n,k >=0
real binom_real(real h, int s);                 // C(h,s) = Π (h-j)/(s)!  j=0..s-1

}  // namespace mt
