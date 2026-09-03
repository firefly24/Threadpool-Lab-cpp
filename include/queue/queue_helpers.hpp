#pragma once

#include <cstddef>
#include <atomic>

// define cache line padded atomic  indexes tp avoid false sharing impact
struct alignas(64) PaddedAtomicIdx{
    std::atomic<std::size_t> index;
    explicit PaddedAtomicIdx(std::size_t idx): index(idx){}    
};

enum class QueueTopology {
	Centralized,
	PerWorker,
};
