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

#include "queue/concurrent_queue.hpp"

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

    // Disable copying
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    // Disable moving
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    // Worker thread function to pop tasks from the queue and execute them
    void startWorkerThread() noexcept;
    
    // for debug counters
    std::atomic<unsigned int> submitted;
    std::atomic<unsigned int> consumed;

public:

	explicit ThreadPool(std::size_t task_capacity, std::size_t max_workers);

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
ThreadPool<Task>::ThreadPool(std::size_t task_capacity, std::size_t max_workers) :
					maxWorkers(max_workers), 
					task_queue_(task_capacity), 
					work_items(0) /*,
					submitted(0),
					consumed(0) */
{
    completed_tasks = 0;
    stop_requested_.store(false, std::memory_order_release);
    
    // Initialize worker threads
    for (size_t worker = 0; worker < maxWorkers; worker++)
        worker_threads.emplace_back([this,worker]() 
        	{ 
        		std::string worker_name = "TPWorker-"+ std::to_string(worker);
    			pthread_setname_np(pthread_self(), worker_name.c_str());
        		this->startWorkerThread(); 
        	}
        ); 
                                 // lamba fn to pop a task from queue to execute
}


template<typename Task>
bool ThreadPool<Task>::taskSubmit(Task& task)
{
    if ( stop_requested_.load(std::memory_order_acquire) )
        return false;
    
    if (task_queue_.tryPush(task) )
    {
    	//submitted++;
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
    	//submitted++;
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
    if (!stop_requested_.exchange(true))
        stopPool();
    
	//std::cout << "ThreadPool stopped" << std::endl;
}


// Worker thread function to pop tasks from the queue and execute them
template<typename Task>
void ThreadPool<Task>::startWorkerThread() noexcept
{
    Task task;
    
    //std::cout << "Starting thread: " << std::hex << std::this_thread::get_id() << std::endl;
    
    // keep polling for new tasks on this thread
    while (1)
    {
        // Wait until there is a task in the queue
		work_items.acquire();
		//consumed++;

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
            task();
        }
        catch (...)
        {
            std::cerr << "Task thown exception" << std::endl;
        }
        // Notify that a task has been completed
        completed_tasks++;
    }
    
stop_worker:
	//std::cout << "Exit thread: " << std::hex << std::this_thread::get_id() << std::endl;
	return;
	
}




#endif /* SIMPLE_THREADPOOL_H */
