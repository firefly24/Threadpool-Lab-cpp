#ifndef SIMPLE_THREADPOOL_H
#define SIMPLE_THREADPOOL_H
#include <thread>
#include <atomic>
#include <iostream>
#include <vector>
#include <semaphore>
#include <climits>
#include <cassert>
#include <utility>
#include <pthread.h>
#include <string>
#include <cstddef>

#include "queue/concurrent_queue.hpp"
#include "./instrumentation/tracing.hpp"

template <typename Task>
class ThreadPool
{
private:
    
    
    ConcurrentQueue<Task> task_queue_;
    std::size_t max_workers_;
    std::size_t task_batch_size_;
    std::counting_semaphore<INT_MAX> work_items_;
    std::atomic<unsigned int> completed_tasks_;
    std::atomic<bool> stop_requested_;
    std::atomic<bool> threadpool_stopped_;
     
    std::vector<std::thread> worker_threads_;
   
    

    // Disable copying
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    // Disable moving
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    // Worker thread function to pop tasks from the queue and execute them
    void startWorkerThread() noexcept;
       
    // Worker thread function to pop tasks in a batch from the queue and execute them
    void startWorkerThreadBatched() noexcept;

public:

	// lauch threadpool, if batch size not provided, keep default 1 task per pop behavior
	explicit ThreadPool(std::size_t task_capacity, std::size_t max_workers, std::size_t batch_size =1);

    unsigned int completedTaskCount()
    {
        return completed_tasks_.load(std::memory_order_relaxed);
    }
    
    // lvalue overload- for Fire and forget tasks with no return value
    bool taskSubmit(Task& task);
    
    // rvalue overload- for Fire and forget tasks with no return value
    bool taskSubmit(Task&& task);
    
    // stopPool will only be used for graceful shutdown, it is idempotent
    void stopPool();
    
    // TODO: I'll add another function for emergency terminate later
    
    // Destructor
    ~ThreadPool();
};



template<typename Task>
ThreadPool<Task>::ThreadPool(std::size_t task_capacity, std::size_t max_worker,std::size_t batch_size) :
					task_queue_(task_capacity),
					max_workers_(max_worker),  
					task_batch_size_(batch_size),
					work_items_(0),
					completed_tasks_(0),
					stop_requested_(false),
					threadpool_stopped_(false)				
{
    // Initialize worker threads
    for (size_t worker = 0; worker < max_workers_; worker++)
        worker_threads_.emplace_back([this,worker]() 
        	{ 
        		std::string worker_name = "TPWorker-"+ std::to_string(worker);
    			pthread_setname_np(pthread_self(), worker_name.c_str());
    			
    			if (task_batch_size_ > 1)
        			this->startWorkerThreadBatched();
        		else
        			this->startWorkerThread();
        	}
        ); 
}


template<typename Task>
bool ThreadPool<Task>::taskSubmit(Task& task)
{
	// possible race where task can still submit after stop requested
    if (!stop_requested_.load(std::memory_order_acquire)  && task_queue_.tryPush(task) )
    {
    	work_items_.release();
    	return true;
    }
    	
    return false;
}


template<typename Task>
bool ThreadPool<Task>::taskSubmit(Task&& task)
{
	// possible race where task can still submit after stop requested
    if (!stop_requested_.load(std::memory_order_acquire) && task_queue_.tryPush(std::move(task)) ) 
    {
    	work_items_.release();
    	return true;
    }
    
	return false;
}

// shutdown the threadpool gracefully on request
template<typename Task>
void ThreadPool<Task>::stopPool()
{
	if (threadpool_stopped_.load(std::memory_order_acquire))
		return;
		
	// notify threadpool to stop accepting new work
	stop_requested_.store(true, std::memory_order_release);
	
	// Notify all worker threads to wake up and return now as queue is empty
	for (std::size_t worker=0; worker<max_workers_; worker++)
		work_items_.release();
		
	// Join all worker threads
	for (auto &worker : worker_threads_)
	{
		if (worker.joinable())
			worker.join();
	}
	
	threadpool_stopped_.store(true,std::memory_order_release);
}


template<typename Task>
ThreadPool<Task>::~ThreadPool()
{	
    // Stop the pool if pool is still running
    if(!threadpool_stopped_.load(std::memory_order_acquire))
    	stopPool();
}


// Worker thread function to pop tasks from the queue and execute them
template<typename Task>
void ThreadPool<Task>::startWorkerThread() noexcept
{
    Task task;
    
    // keep polling for new tasks on this thread
    while (true)
    {
        // Wait until there is a task in the queue
		work_items_.acquire();

		// return only if pool is stopped and all tasks are completed
		if (stop_requested_.load(std::memory_order_acquire))
		{
			if(task_queue_.empty())
				goto stop_worker;		
		}
			
        // Pop a task from the queue to attach to current worker thread
        if ( !task_queue_.tryPop(task) )
        {
        	// pop failure due to queue empty is only ok during pool termination
        	if (stop_requested_.load(std::memory_order_acquire))
        		goto stop_worker;
			
			// this should almost never fail now, will add error log/event log later
        	assert(false);
        	goto stop_worker;
        }

        // Execute the task
        try
        {
        	TP_TRACE_EVENT("ExecuteTask");
            task();
        }
        catch (...)
        {
            std::cerr << "Task thown exception" << std::endl;
        }
        // Notify that a task has been completed
        completed_tasks_.fetch_add(1,std::memory_order_relaxed);
    }
    
stop_worker:
	//I want to retain this label as a single point where all early exits arrive, rather than sprinkling returns everywhere
	return;
	
}

template<typename Task>
void ThreadPool<Task>::startWorkerThreadBatched() noexcept
{
    Task task;

    // Add per thread batching infrastrructure
    std::vector<Task> local_batch;
    local_batch.reserve(task_batch_size_);
    std::size_t work_permits = 0;
    std::size_t pulled_work = 0;
   
    // keep polling for new tasks on this thread
    while (true)
    {
    	local_batch.clear();
		work_permits = 0;
		pulled_work = 0;
		
        // Wait until there is a task in the queue
		work_items_.acquire();
		work_permits =1;
		
		// Take n work permits optimistically
		while((work_permits < task_batch_size_) && work_items_.try_acquire())
			work_permits++;
		
		//batch pop from the queue
		pulled_work = task_queue_.tryPopBatch(local_batch, work_permits);
		
		// This assert checks for any discrepancy in the semaphore- work_permit contract
		// The work permits can only be greater than pulled work when it consumes shutdown permits in valid case
		assert(stop_requested_.load(std::memory_order_acquire) || (pulled_work == work_permits) );
		
		// Execute the task batch 
		for(std::size_t idx = 0; idx < pulled_work; idx++)
		{
		    try
		    {
		    	TP_TRACE_EVENT("ExecuteTask");
		        task = std::move(local_batch[idx]);
		        task();
		    }
		    catch (...)
		    {
		        std::cerr << "Task thown exception" << std::endl;
		    }   
		}
		
		// Update completed task count
        completed_tasks_.fetch_add(pulled_work,std::memory_order_relaxed);
		
		// return only if pool is stopped and all tasks are completed
		if (stop_requested_.load(std::memory_order_acquire))
		{
			std::size_t shutdown_permits_consumed = work_permits - pulled_work;
			while(shutdown_permits_consumed)
        	{
        		// some additional permits emitted for stop may have been consumed by this thread, re-emit them to unblock other threads and exit
        		shutdown_permits_consumed--;
        		work_items_.release();
        	}
        	
			if(task_queue_.empty())
			{	
				// stop is requested and no remaining task, we can safely retire this worker thread
				return;		
			}
		}
    }
    
	return;
	
}



#endif /* SIMPLE_THREADPOOL_H */
