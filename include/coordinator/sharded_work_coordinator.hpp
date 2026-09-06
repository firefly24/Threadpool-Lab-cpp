#pragma once
#ifndef SHARDED_COORDINATOR_H
#define SHARDED_COORDINATOR_H

#include <vector>
#include <atomic>
#include <climits>
#include <cassert>
#include <utility>
#include <cstddef>
#include <memory>

// TODO : include headers for QueueContainer and QueueTopology
#include "../queue/SPSC_queue/spsc_lockfree.hpp"

template <typename Task>
struct SPSCShard
{
	SPSCQueue<Task> queue_;							// queue storage
	std::counting_semaphore<INT_MAX> new_work_;		// work permits for consuming work 
	
	explicit SPSCShard(std::size_t capacity): queue_(capacity),
													new_work_(0)
	{
	
	}
	
		/*
	std::size_t id_;								// shard id
	std::size_t capacity_;							// queue capacity
	std::atomic<bool> drained_;						// shutdown/alive state control
	*/ 
};




template <typename Task>
class ShardedWorkCoordinator
{

private:
	std::size_t max_workers_;
	
	// Queue container
	std::vector<std::unique_ptr<SPSCShard<Task>>> work_queues_;
	
	// routing state, non-atomic as we're expecting only single producer to increment it sequentially
	std::size_t next_worker_;
	
	// TODO: Define shared drain state - will it be per-shard or global ? 
	
	// TODO: define task routing state
	
	// TODO: define signalling state object/policy
	
	// TODO: define any queue<-> worker coordination objects
	
	// TODO: Waiting mechanism for workers
	
	// TODO: Work distribution policy
	
	// TODO: shutdown helpers
	
	
	
	void createQueueShards(std::size_t total_capacity)
	{
	
		std::size_t perworker_capacity = total_capacity/max_workers_;
		std::size_t leftover = total_capacity % max_workers_;
	
		work_queues_.reserve(max_workers_);	
	
		for (std::size_t shard = 0; shard < max_workers_; shard++)
		{
			std::size_t per_queue_capacity = perworker_capacity 
										+ ((shard < leftover)?1:0);
										
			work_queues_.push_back(std::make_unique<SPSCShard<Task>>(per_queue_capacity));
		}
	}
	
	
	void advanceShard()
	{
		next_worker_ = (next_worker_ + 1)% max_workers_;
	}

public:

	// Disable copying
    ShardedWorkCoordinator(const ShardedWorkCoordinator &) = delete;
    ShardedWorkCoordinator &operator=(const ShardedWorkCoordinator &) = delete;

    // Disable moving
    ShardedWorkCoordinator(ShardedWorkCoordinator &&) = delete;
    ShardedWorkCoordinator &operator=(ShardedWorkCoordinator &&) = delete;
	

	ShardedWorkCoordinator(std::size_t task_capacity, std::size_t max_workers);
	
	// Currently only single producer allowed to submit tasks
	bool submit(Task &&task);
	bool submit(Task &task);
	
	std::size_t acquireWorkBlocking(std::size_t worker_id, Task& out_buffer);
	
	void wakeAllWorkers();
};


template <typename Task>
ShardedWorkCoordinator<Task>::ShardedWorkCoordinator(std::size_t task_capacity, 
													 std::size_t max_workers ) :
														max_workers_(max_workers),
														next_worker_(0)
{
		createQueueShards(task_capacity);
}


template <typename Task>
std::size_t ShardedWorkCoordinator<Task>::acquireWorkBlocking(std::size_t worker_id, Task& out_buffer)
{
	std::size_t acquired_count = 0;
	
	// if shard has no work, block 
	work_queues_[worker_id]->new_work_.acquire();
	
	// pop 1 work
	if ( (work_queues_[worker_id]->queue_).tryPop(out_buffer) )
		acquired_count++;
	
	// tryPop must only fail when queue is completely drained for shutdown		

	return acquired_count;

}


template <typename Task>
bool ShardedWorkCoordinator<Task>::submit(Task& task)
{
	bool ret = false;
	
	// choose shard
	std::size_t shard = next_worker_;
	 
	 
	// attempt admission to that shard
	if ( (ret = work_queues_[shard]->queue_.tryPush(task)) )
	{
		// if accepted, signal notify on that shard
		work_queues_[shard]->new_work_.release();
	}
	
	// Update routing state
	advanceShard();
	
	// return accepted/ rejected
	return ret;
}

template <typename Task>
bool ShardedWorkCoordinator<Task>::submit(Task&& task)
{
	bool ret = false;
	
	// choose shard
	std::size_t shard = next_worker_;
	
	// attempt admission to that shard
	if ( (ret = work_queues_[shard]->queue_.tryPush(std::move(task))) )
	{
		// if accepted, signal notify on that shard
		work_queues_[shard]->new_work_.release();
	}
	
	// Update routing state
	advanceShard();
	
	// return accepted/ rejected
	return ret;
}


template <typename Task>
void ShardedWorkCoordinator<Task>::wakeAllWorkers()
{
	for (std::size_t shard = 0; shard < max_workers_; shard++)
	{									
			(work_queues_[shard]->new_work_).release();
	}

}



/*
Template <typename Task>
class ShardedQueueContainer{

private:
	std::size_t shards_;
	std::size_t total_capacity_;
	std::vector<std::unique_ptr<SPSCQueueShard<Task>>> work_queues_;
public: 
	
	explicit ShardedQueueContainer<Task>(std::size_t shards, std::size_t capacity):
									shards_(shards), total_capacity_(capacity)
	{
	
		std::size_t perworker_capacity = total_capacity_/shards_;
		std::size_t leftover = total_capacity_ % shards_;
	
		work_queues_.reserve(shards_);	
	
		for (std::size_t shard = 0; shard < shards_; shard++)
		{
			std::size_t per_queue_capacity = perworker_capacity 
										+ ((shard < leftover)?1:0);
										
			work_queues_.push_back(std::make_unique<SPSCQueueShard<Task>>(shard, per_queue_capacity));
		}
	}
};

*/

#endif /* SHARDED_COORDINATOR_H */


