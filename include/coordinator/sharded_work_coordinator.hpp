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
#include <semaphore>

// TODO : include headers for QueueContainer and QueueTopology
#include "../queue/SPSC_queue/spsc_lockfree.hpp"

template <typename Task>
struct SPSCShard
{
	SPSCQueue<Task> queue_;							// queue storage
	//std::counting_semaphore<INT_MAX> new_work_;		// work permits for consuming work 
	//std::size_t batch_size_;
	std::atomic<std::size_t> has_work_;
	
	explicit SPSCShard(std::size_t capacity):queue_(capacity),
											 has_work_(0)
											
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
	
	std::size_t producer_batch_size;
	
	std::vector<std::size_t> worker_batch_size_;
	
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
	
	bool submitBatch(std::vector<Task> tasks, std::size_t batch_size);
	
	void setWorkerBatchSize(std::size_t worker_id,std::size_t max_batch);
	
	std::size_t acquireWorkBlocking(std::size_t worker_id /*, std::vector<Task>& out_buffer*/ );
	
	void wakeAllWorkers();
};


template <typename Task>
ShardedWorkCoordinator<Task>::ShardedWorkCoordinator(std::size_t task_capacity, 
													 std::size_t max_workers ) :
														max_workers_(max_workers),
														next_worker_(0),
														worker_batch_size_(max_workers,1)
{
		createQueueShards(task_capacity);
}


template <typename Task>
std::size_t ShardedWorkCoordinator<Task>::acquireWorkBlocking(std::size_t worker_id
															/*,std::vector<Task>& out_buffer*/)
{
	// reason why Task default_initializable constraint required
	Task task;
	std::size_t tasks_executed = 0;
	
	SPSCShard<Task>& local_shard = *work_queues_[worker_id];

	local_shard.has_work_.wait(0, std::memory_order_acquire);
	
	while( (work_queues_[worker_id]->queue_).tryPop(task) )
	{	
		// Execute the task
		try
		{
			TP_TRACE_EVENT("ExecuteTask");
    		task();
    		//local_shard.has_work_.fetch_sub(1, std::memory_order_relaxed);
		}
		catch (...)
		{
    		std::cerr << "Task thown exception" << std::endl;
		}
		tasks_executed++; 
	}
	local_shard.has_work_.fetch_sub(tasks_executed,std::memory_order_release);
	
	return tasks_executed;
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
		work_queues_[shard]->has_work_.fetch_add(1,std::memory_order_release);
		work_queues_[shard]->has_work_.notify_one();
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
		work_queues_[shard]->has_work_.fetch_add(1,std::memory_order_release);
		work_queues_[shard]->has_work_.notify_one();
	}
	
	// Update routing state
	advanceShard();
	
	// return accepted/ rejected
	return ret;
}

template <typename Task>
bool ShardedWorkCoordinator<Task>::submitBatch(std::vector<Task> tasks, std::size_t batch_size)
{
	bool ret = false;
	std::size_t idx = 0;
		
	// choose shard
	std::size_t shard = next_worker_;
	
	assert(batch_size <= tasks.size());
	
	while(shard == next_worker_)
	{
		SPSCShard<Task>& local_shard = *work_queues_[shard];
		
		while( idx < batch_size)
		{
			// attempt admission to that shard
			if (  work_queues_[shard]->queue_.tryPush(tasks[idx]) )
			{
				idx++;
				local_shard.has_work_.fetch_add(1,std::memory_order_release);
				//local_shard.has_work_.notify_one();
			}
			else
			{
				advanceShard();
				shard = next_worker_;
			}
		}
		local_shard.has_work_.notify_one();
		
		if(idx == batch_size)
		{
			advanceShard();
			break;
		}
	}	
	return true;
}


template <typename Task>
void ShardedWorkCoordinator<Task>::wakeAllWorkers()
{
	for (std::size_t shard = 0; shard < max_workers_; shard++)
	{									
			//(work_queues_[shard]->new_work_).release();
			work_queues_[shard]->has_work_.fetch_add(1,std::memory_order_release);
			work_queues_[shard]->has_work_.notify_one();
	}

}

template <typename Task>
void ShardedWorkCoordinator<Task>::setWorkerBatchSize(std::size_t worker_id,std::size_t max_batch)
{
	worker_batch_size_[worker_id] = max_batch;
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


