#pragma once
// Pinned (page-locked) host buffer. Required for cudaMemcpyAsync overlap with kernels.
// Implementation in src/cuda/stream_pool.cpp (and friends) calls cudaHostAlloc.

#include <cstddef>
#include <utility>

namespace mt {

template <class T>
class PinnedBuffer {
public:
    PinnedBuffer() = default;
    explicit PinnedBuffer(std::size_t n);
    PinnedBuffer(const PinnedBuffer&) = delete;
    PinnedBuffer& operator=(const PinnedBuffer&) = delete;
    PinnedBuffer(PinnedBuffer&& o) noexcept : ptr_(o.ptr_), n_(o.n_) { o.ptr_ = nullptr; o.n_ = 0; }
    PinnedBuffer& operator=(PinnedBuffer&& o) noexcept {
        if (this != &o) { reset(); ptr_ = o.ptr_; n_ = o.n_; o.ptr_ = nullptr; o.n_ = 0; }
        return *this;
    }
    ~PinnedBuffer() { reset(); }

    T*       data()       noexcept { return ptr_; }
    const T* data() const noexcept { return ptr_; }
    std::size_t size() const noexcept { return n_; }

    void reset();
private:
    T*          ptr_ = nullptr;
    std::size_t n_   = 0;
};

template <class T>
PinnedBuffer<T> make_pinned_buffer(std::size_t n) { return PinnedBuffer<T>(n); }

}  // namespace mt
