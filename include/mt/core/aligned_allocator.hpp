#pragma once
#include <cstddef>
#include <cstdlib>
#include <new>
#include <type_traits>

namespace mt {

// 64-byte aligned host allocator (cache line + AVX-512 friendly).
template <class T, std::size_t Align = 64>
struct AlignedAllocator {
    using value_type = T;
    AlignedAllocator() noexcept = default;
    template <class U> AlignedAllocator(const AlignedAllocator<U, Align>&) noexcept {}

    T* allocate(std::size_t n) {
        if (n == 0) return nullptr;
        void* p = nullptr;
#if defined(_MSC_VER)
        p = _aligned_malloc(n * sizeof(T), Align);
        if (!p) throw std::bad_alloc();
#else
        if (posix_memalign(&p, Align, n * sizeof(T)) != 0) throw std::bad_alloc();
#endif
        return static_cast<T*>(p);
    }
    void deallocate(T* p, std::size_t) noexcept {
#if defined(_MSC_VER)
        _aligned_free(p);
#else
        std::free(p);
#endif
    }
    template <class U> bool operator==(const AlignedAllocator<U, Align>&) const noexcept { return true; }
    template <class U> bool operator!=(const AlignedAllocator<U, Align>&) const noexcept { return false; }
};

}  // namespace mt
