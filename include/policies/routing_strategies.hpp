#pragma once 

#include <cstddef>

class RoundRobinRouting {

private:

std::size_t shard_count_;
std::size_t next_shard_;

public:
    RoundRobinRouting(std::size_t shards) : shard_count_(shards), next_shard_(0)
    {

    }

    std::size_t nextShard()
    {
        std::size_t curr_shard = next_shard_;
        next_shard_ = (next_shard_ + 1) % shard_count_;
        return curr_shard;
    }
};

template <std::size_t ChunkSize>
class ChunkyRoundRobinRouting {

    private:
    
    std::size_t shard_count_;
    std::size_t next_shard_;
    std::size_t chunk_size_ = ChunkSize;
    std::size_t chunk_quota_;
    
    public:
        ChunkyRoundRobinRouting(std::size_t shards) : shard_count_(shards), next_shard_(0)
        {
            chunk_quota_= chunk_size_;
        }
    
        std::size_t nextShard()
        {
            if (!chunk_quota_)
            {
                next_shard_ = (next_shard_ + 1) % shard_count_;
                chunk_quota_ = chunk_size_;
            }

            chunk_quota_--;
            return next_shard_;
        }
    };