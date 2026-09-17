#pragma once
#include <cstddef>
#include <vector>

namespace mt {

// Fixed-capacity host ring buffer for streaming feature/state windows.
template <class T>
class RingBuffer {
public:
    explicit RingBuffer(std::size_t cap) : data_(cap), cap_(cap) {}
    void push(const T& v) {
        data_[head_] = v;
        head_ = (head_ + 1) % cap_;
        if (size_ < cap_) ++size_;
    }
    std::size_t size() const noexcept { return size_; }
    std::size_t capacity() const noexcept { return cap_; }
    // Indexing: 0 = oldest, size()-1 = newest.
    const T& operator[](std::size_t i) const {
        std::size_t base = (head_ + cap_ - size_) % cap_;
        return data_[(base + i) % cap_];
    }
private:
    std::vector<T> data_;
    std::size_t cap_ = 0, head_ = 0, size_ = 0;
};

}  // namespace mt
