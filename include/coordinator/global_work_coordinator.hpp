#pragma once
#ifndef GLOBAL_COORDINATOR_H
#define GLOBAL_COORDINATOR_H

#include <vector>
#include <atomic>
#include <climits>
#include <cassert>
#include <utility>
#include <cstddef>
#include <memory>
#include <semaphore>

// TODO : include headers for QueueContainer and QueueTopology
#include "../queue/SPMC_locked/spmc_queue.hpp"

template <typename Task>
struct GlobalSPMCQueue
{
	SPMCQueueLocked<Task> queue_;					// queue storage
	std::counting_semaphore<INT_MAX> new_work_;		// work permits for consuming work 
	//std::size_t batch_size_;
	
	explicit GlobalSPMCQueue(std::size_t capacity):	queue_(capacity),
											       	new_work_(0)//,
													//batch_size_(1)
											
	{
	}
	
		/*
	std::size_t id_;								// shard id
	std::size_t capacity_;							// queue capacity
	std::atomic<bool> drained_;						// shutdown/alive state control
	*/ 
};


template <typename Task>
class GlobalWorkCoordinator
{

private:
	std::size_t max_workers_;
	
	// Queue container
	//std::vector<std::unique_ptr<SPSCShard<Task>>> work_queues_;
	//std::unique_ptr<GlobalSPMCQueue<Task>> global_queue_;
	GlobalSPMCQueue<Task> global_spmc_;
	
	// routing state, non-atomic as we're expecting only single producer to increment it sequentially
	//std::size_t next_worker_;
	
	// TODO: Define shared drain state - will it be per-shard or global ? 
	
	// TODO: define task routing state
	
	// TODO: define signalling state object/policy
	
	// TODO: define any queue<-> worker coordination objects
	
	// TODO: Waiting mechanism for workers
	
	// TODO: Work distribution policy
	
	// TODO: shutdown helpers

public:

	// Disable copying
    GlobalWorkCoordinator(const GlobalWorkCoordinator &) = delete;
    GlobalWorkCoordinator &operator=(const GlobalWorkCoordinator &) = delete;

    // Disable moving
    GlobalWorkCoordinator(GlobalWorkCoordinator &&) = delete;
    GlobalWorkCoordinator &operator=(GlobalWorkCoordinator &&) = delete;
	

	GlobalWorkCoordinator(std::size_t task_capacity, std::size_t max_workers);
	
	// Currently only single producer allowed to submit tasks
	bool submit(Task &&task);
	bool submit(Task &task);
	
	bool acquireWorkBlocking(std::size_t worker_id, Task& out);
	
	void notifyDrain();
};


template <typename Task>
GlobalWorkCoordinator<Task>::GlobalWorkCoordinator(std::size_t task_capacity, 
													 std::size_t max_workers ) :
														max_workers_(max_workers),
														global_spmc_(task_capacity)
{
		//createQueueShards(task_capacity);
}


template <typename Task>
bool GlobalWorkCoordinator<Task>::acquireWorkBlocking(std::size_t worker_id,
															  Task& out)
{
	// reason why Task default_initializable constraint required
	Task task;
	
	// if shard has no work, block 
	//work_queues_[worker_id]->new_work_.acquire();
	global_spmc_.new_work_.acquire();

	// tryPop WILL only fail when queue is empty after shutdown requested
	// tryPop will not fail during normal operation, as counting semaphore always ensures queue has data before tryPop
	return  global_spmc_.queue_.tryPop(out);
}


template <typename Task>
bool GlobalWorkCoordinator<Task>::submit(Task& task)
{
	// attempt admission to the global queue
	if ( global_spmc_.queue_.tryPush(task) )
	{
		// if accepted, signal notify to waiting workers
		global_spmc_.new_work_.release();
		return true;
	}
	return false;
}

template <typename Task>
bool GlobalWorkCoordinator<Task>::submit(Task&& task)
{
	// attempt admission to that global queue
	if ( global_spmc_.queue_.tryPush(std::move(task)) )
	{
		// if accepted, signal notify to waiting workers
		global_spmc_.new_work_.release();
		return true;
	}
	return false;
}


template <typename Task>
void GlobalWorkCoordinator<Task>::notifyDrain()
{
	for (std::size_t worker = 0; worker < max_workers_; worker++)
	{									
		global_spmc_.new_work_.release();
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

#endif /* GLOBAL_COORDINATOR_H */


