#pragma once

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
class ShardedWorkCoordinator
{

private:
	std::size_t max_workers_;
	
	//QueueContainer<Task> queue_;	
	std::vector<std::unique_ptr<SPSCQueue<Task>>> work_queues_;
	
	std::size_t next_worker_;
	
	// TODO: define task routing state
	
	// TODO: define signalling state object/policy
	
	// TODO: define any queue<-> worker coordination objects
	
	// TODO: Waiting mechanism for workers
	
	// TODO: Work distribution policy
	
	// TODO: shutdown helpers

public:

	// Disable copying
    ShardedWorkCoordinator(const ShardedWorkCoordinator &) = delete;
    ShardedWorkCoordinator &operator=(const ShardedWorkCoordinator &) = delete;

    // Disable moving
    ShardedWorkCoordinator(ShardedWorkCoordinator &&) = delete;
    ShardedWorkCoordinator &operator=(ShardedWorkCoordinator &&) = delete;
	

	ShardedWorkCoordinator(std::size_t task_capacity, std::size_t max_workers);
	
	bool submit(Task &&task);
	bool submit(Task &task);
	
	std::size_t acquireWorkBlocking(std::size_t worker_id, Task& out_buffer);
	
	void handleShutdown();
};


template <typename Task>
ShardedWorkCoordinator::ShardedWorkCoordinator( std::size_t max_workers,
												std::size_t task_capacity) :
														max_workers_(max_workers)
{
	std::size_t perworker_capacity = task_capacity/max_workers_;
	std::size_t leftover = task_capacity % max_workers_;
	
	work_queues_.reserve(max_workers_);	
	
	for (std::size_t worker = 0; worker < max_workers_; worker++)
	{
		std::size_t per_queue_capacity = perworker_capacity 
									+ ((worker < leftover)?1:0);
									
		work_queues_.push_back(std::make_unique<SPSCQueue<Task>>(per_queue_capacity));
	}
}







