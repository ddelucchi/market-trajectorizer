#pragma once
// Device-resident RAII buffer wrapping cudaMalloc/cudaFree.
// Implementation in src/cuda/stream_pool.cpp.
#include <cstddef>

namespace mt {

template <class T>
class DeviceBuffer {
public:
    DeviceBuffer() = default;
    explicit DeviceBuffer(std::size_t n);
    DeviceBuffer(const DeviceBuffer&) = delete;
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;
    DeviceBuffer(DeviceBuffer&& o) noexcept : ptr_(o.ptr_), n_(o.n_) { o.ptr_ = nullptr; o.n_ = 0; }
    DeviceBuffer& operator=(DeviceBuffer&& o) noexcept {
        if (this != &o) { reset(); ptr_ = o.ptr_; n_ = o.n_; o.ptr_ = nullptr; o.n_ = 0; }
        return *this;
    }
    ~DeviceBuffer() { reset(); }

    T*       data()       noexcept { return ptr_; }
    const T* data() const noexcept { return ptr_; }
    std::size_t size() const noexcept { return n_; }
    void reset();
private:
    T*          ptr_ = nullptr;
    std::size_t n_   = 0;
};

template <class T>
DeviceBuffer<T> make_device_buffer(std::size_t n) { return DeviceBuffer<T>(n); }

}  // namespace mt
