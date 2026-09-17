#pragma once
#include <chrono>
#include <string>

namespace mt {

class Timer {
public:
    Timer() : t0_(std::chrono::steady_clock::now()) {}
    double seconds() const {
        using namespace std::chrono;
        return duration<double>(steady_clock::now() - t0_).count();
    }
    void reset() { t0_ = std::chrono::steady_clock::now(); }
private:
    std::chrono::steady_clock::time_point t0_;
};

}  // namespace mt
