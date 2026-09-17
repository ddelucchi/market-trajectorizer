#pragma once
#include <array>
#include <cstddef>

namespace mt {

template <class T, std::size_t N>
using FixedVector = std::array<T, N>;

}
