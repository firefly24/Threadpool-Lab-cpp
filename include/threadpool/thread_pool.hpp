#ifndef SIMPLE_THREADPOOL_H
#define SIMPLE_THREADPOOL_H
#include <thread>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <vector>
#include <future>
#include <semaphore>
#include <climits>
#include <cassert>
#include <chrono>

#include "queue/concurrent_queue.hpp"
#include "./instrumentation/tracing.hpp"

#define TASK_BATCH_SIZE 4

using namespace std;

//typedef std::function<void()> T;

template <typename Task>
class ThreadPool
{
private:
    //std::size_t capacity;
    std::size_t maxWorkers;
    
    ConcurrentQueue<Task> task_queue_;
    std::counting_semaphore<INT_MAX> work_items;
    std::vector<std::thread> worker_threads;
    std::atomic<unsigned int> completed_tasks;
    std::atomic<bool> stop_requested_;
    std::size_t task_batch_size_;

    // Disable copying
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    // Disable moving
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    // Worker thread function to pop tasks from the queue and execute them
    void startWorkerThread() noexcept;
    
    
    // Worker thread function to pop tasks from the queue and execute them
    void startWorkerThreadBatched() noexcept;
    
    // for debug counters
    //std::atomic<unsigned int> submitted;
    //std::atomic<unsigned int> consumed;

public:

	explicit ThreadPool(std::size_t task_capacity, std::size_t max_workers, std::size_t batch_size=TASK_BATCH_SIZE);

    int completedTaskCount()
    {
        return completed_tasks.load();
    }
    
    // lvalue overload- for Fire and forget tasks with no return value
    bool taskSubmit(Task& task);
    
    // rvalue overload- for Fire and forget tasks with no return value
    bool taskSubmit(Task&& task);
    
    // stopPool will only be used for graceful shutdown
	// TODO: I'll add another function for emergency terminate later
    void stopPool();
    
    ~ThreadPool();
};



template<typename Task>
ThreadPool<Task>::ThreadPool(std::size_t task_capacity, std::size_t max_workers,std::size_t batch_size) :
					maxWorkers(max_workers), 
					task_queue_(task_capacity), 
					work_items(0),
					task_batch_size_(batch_size) /*,
					submitted(0),
					consumed(0)*/
{
    completed_tasks = 0;
    stop_requested_.store(false, std::memory_order_release);
    
    // Initialize worker threads
    for (size_t worker = 0; worker < maxWorkers; worker++)
        worker_threads.emplace_back([this,worker]() 
        	{ 
        		std::string worker_name = "TPWorker-"+ std::to_string(worker);
    			pthread_setname_np(pthread_self(), worker_name.c_str());
        		//this->startWorkerThread(); 
        		this->startWorkerThreadBatched();
        	}
        ); 
}


template<typename Task>
bool ThreadPool<Task>::taskSubmit(Task& task)
{
    if ( stop_requested_.load(std::memory_order_acquire) )
        return false;
    
    if (task_queue_.tryPush(task) )
    {
    	work_items.release();
    	return true;
    }
    	
    return false;
}


template<typename Task>
bool ThreadPool<Task>::taskSubmit(Task&& task)
{
	if ( stop_requested_.load(std::memory_order_acquire) )
        return false;
    
    if (task_queue_.tryPush(std::move(task)) ) 
    {
    	work_items.release();
    	return true;
    }
    
	return false;
}


template<typename Task>
void ThreadPool<Task>::stopPool()
{
	// notify threadpool to stop accepting new work
    stop_requested_.store(true, std::memory_order_release);
	
	// Notify all worker threads to wake up and return now as queue is empty
	// TODO: refine later if this belongs here or in destructor
	for (std::size_t worker=0; worker<=maxWorkers; worker++)
	{
		work_items.release();
	}
		
	// Join all worker threads
    for (auto &worker : worker_threads)
    {
    	if (worker.joinable())
        	worker.join();
    }
	
}


template<typename Task>
ThreadPool<Task>::~ThreadPool()
{	
    // Stop the pool and notify all worker threads
    if(!stop_requested_.exchange(true))
    	stopPool();
    
    /*
    std::cout << "ThreadPool stopped" << std::endl;
	*/
	std::cout << "Total work consumed: " << completed_tasks.load(std::memory_order_relaxed) << std::endl;
	
	std::cout << "Push contention: " << task_queue_.push_stats.load(std::memory_order_relaxed) << std::endl;
	std::cout << "Pop contention: " << task_queue_.pop_stats.load(std::memory_order_relaxed) << std::endl;

	double total_pop_wait_time = std::chrono::duration<double>(task_queue_.pop_contention_time).count();
	
	std::cout << "Pop wait duration: " << total_pop_wait_time *1000   << "ms" <<std::endl;
	
	std::cout << "Total batching events: " << task_queue_.batches_count.load(std::memory_order_relaxed) << std::endl;
	
}


// Worker thread function to pop tasks from the queue and execute them
template<typename Task>
void ThreadPool<Task>::startWorkerThread() noexcept
{
    Task task;
    
    
    // Add per thread batching infrastrructure
    std::vector<Task> local_batch;
    local_batch.reserve(task_batch_size_);
    
    //std::cout << "Starting thread: " << std::hex << std::this_thread::get_id() << std::endl;
    
    // keep polling for new tasks on this thread
    while (1)
    {
        // Wait until there is a task in the queue
		work_items.acquire();

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
        completed_tasks.fetch_add(1,std::memory_order_relaxed);
    }
    
stop_worker:
	//std::cout << "Exit thread: " << std::hex << std::this_thread::get_id() << std::endl;
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
    
    //std::cout << "Starting thread: " << std::hex << std::this_thread::get_id() << std::endl;
    
    // keep polling for new tasks on this thread
    while (1)
    {
    	local_batch.clear();
		work_permits = 0;
		pulled_work = 0;
		
        // Wait until there is a task in the queue
		work_items.acquire();
		work_permits =1;
		
		// Take n work permits optimistically
		while((work_permits < task_batch_size_) && work_items.try_acquire())
			work_permits++;
		
		//batch pop from the queue
		pulled_work = task_queue_.tryPopBatch(local_batch, work_permits);
		
		assert(stop_requested_.load(std::memory_order_acquire) || (pulled_work == work_permits) );
		
		for(std::size_t idx = 0; idx < pulled_work; idx++)
		{
			// Execute the task
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
        completed_tasks.fetch_add(pulled_work,std::memory_order_relaxed);
		
		// return only if pool is stopped and all tasks are completed
		if (stop_requested_.load(std::memory_order_acquire))
		{
		
			if (pulled_work < work_permits)
        	{
        		pulled_work++;
        		work_items.release();
        	}
			if(task_queue_.empty())
				goto stop_worker;		
		}
    }
    
stop_worker:
	//std::cout << "Exit thread: " << std::hex << std::this_thread::get_id() << std::endl;
	return;
	
}



#endif /* SIMPLE_THREADPOOL_H */
