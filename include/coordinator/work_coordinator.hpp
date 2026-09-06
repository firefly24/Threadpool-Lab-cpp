#pragma once

#include <vector>
#include <atomic>
#include <climits>
#include <cassert>
#include <utility>
#include <cstddef>

// TODO : include headers for QueueContainer and QueueTopology

template <typename Task>
class WorkCoordinator
{

private:
	std::size_t task_capacity_;
	std::size_t max_workers_;
	
	QueueTopology queue_type_; // leave this for now, decide later
	QueueContainer<Task> queue_;	
	
	std::size_t next_worker_;
	
	// TODO: define task routing state
	
	// TODO: define signalling state object/policy
	
	// TODO: define any queue<-> worker coordination objects
	
	// TODO: Waiting mechanism for workers
	
	// TODO: Work distribution policy
	
	// TODO: shutdown helpers
	
	


public:

	// Disable copying
    WorkCoordinator(const WorkCoordinator &) = delete;
    WorkCoordinator &operator=(const WorkCoordinator &) = delete;

    // Disable moving
    WorkCoordinator(WorkCoordinator &&) = delete;
    WorkCoordinator &operator=(WorkCoordinator &&) = delete;
	

	WorkCoordinator(std::size_t task_capacity, std::size_t max_workers);
	
	bool submit(Task &&task);
	bool submit(Task &task);
	
	std::size_t acquireWorkBlocking(std::size_t worker_id, Task& out_buffer);
	
	void handleShutdown();
};










