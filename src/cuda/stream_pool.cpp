#include "mt/cuda/stream_pool.hpp"
#include "mt/cuda/cuda_check.hpp"

namespace mt {

StreamPool::StreamPool() {
    for (int i = 0; i < static_cast<int>(StreamId::Count); ++i) {
        MT_CUDA_CHECK(cudaStreamCreateWithFlags(&streams_[i], cudaStreamNonBlocking));
    }
}

StreamPool::~StreamPool() {
    for (int i = 0; i < static_cast<int>(StreamId::Count); ++i) {
        if (streams_[i]) cudaStreamDestroy(streams_[i]);
    }
}

cudaStream_t StreamPool::get(StreamId id) const {
    return streams_[static_cast<int>(id)];
}

void StreamPool::synchronize_all() const {
    for (int i = 0; i < static_cast<int>(StreamId::Count); ++i) {
        if (streams_[i]) MT_CUDA_CHECK(cudaStreamSynchronize(streams_[i]));
    }
}

}  // namespace mt
