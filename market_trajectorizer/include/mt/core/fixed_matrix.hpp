#pragma once
#include <array>
#include <cstddef>

namespace mt {

template <class T, std::size_t R, std::size_t C>
struct FixedMatrix {
    std::array<T, R * C> a{};
    constexpr T& operator()(std::size_t i, std::size_t j)       { return a[i * C + j]; }
    constexpr const T& operator()(std::size_t i, std::size_t j) const { return a[i * C + j]; }
    static constexpr std::size_t rows() { return R; }
    static constexpr std::size_t cols() { return C; }
};

}  // namespace mt
