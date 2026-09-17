#pragma once
#include "mt/engine/execution_graph.hpp"
#include <cuda_runtime.h>

namespace mt {

class StreamPool {
public:
    StreamPool();
    ~StreamPool();
    StreamPool(const StreamPool&) = delete;
    StreamPool& operator=(const StreamPool&) = delete;

    cudaStream_t get(StreamId id) const;
    void synchronize_all() const;
private:
    cudaStream_t streams_[static_cast<int>(StreamId::Count)] {};
};

}  // namespace mt
