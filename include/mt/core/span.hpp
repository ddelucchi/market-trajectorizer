#pragma once
#include <cstddef>
#include <span>

namespace mt {
// Thin re-export so call sites can write mt::span<T> without dragging <span> everywhere.
template <class T, std::size_t E = std::dynamic_extent>
using span = std::span<T, E>;
}
