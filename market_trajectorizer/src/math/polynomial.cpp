#include "mt/math/polynomial.hpp"
#include <algorithm>
namespace mt {

cplx poly_eval(const Vec<cplx>& c, cplx z) {
    cplx acc{};
    for (auto it = c.rbegin(); it != c.rend(); ++it) acc = acc * z + *it;
    return acc;
}

Vec<cplx> poly_mul(const Vec<cplx>& a, const Vec<cplx>& b) {
    if (a.empty() || b.empty()) return Vec<cplx>{};
    Vec<cplx> r(a.size() + b.size() - 1, cplx{});
    for (usize i = 0; i < a.size(); ++i)
        for (usize j = 0; j < b.size(); ++j)
            r[i + j] += a[i] * b[j];
    return r;
}

Vec<cplx> poly_add(const Vec<cplx>& a, const Vec<cplx>& b) {
    Vec<cplx> r(std::max(a.size(), b.size()), cplx{});
    for (usize i = 0; i < a.size(); ++i) r[i] += a[i];
    for (usize i = 0; i < b.size(); ++i) r[i] += b[i];
    return r;
}

}
