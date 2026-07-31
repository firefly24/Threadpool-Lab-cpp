#ifndef SIMPLE_THREADPOOL_H
#define SIMPLE_THREADPOOL_H
#include <thread>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <vector>
#include <future>
#include <semaphore>
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
    
    /*

    // Push a task into the queue, and return a future object which other programs can use to fetch results of the task
    // need to declare the return type as std::future explicitly, as auto is deducing it as "void"
    template <typename Func, typename... Args>
    auto pushTask(Func &&func, Args &&...args) -> std::future<decltype(func(args...))>
    {
        // We need to encapsulate the function such that calling func() will be equivalent to calling func(args...)
        // To do this, we can bind the args.. to func object by: auto task =  std::bind(func,args...);
        // But, we need to preserve the lvalue/rvalue typeof arguments, so we need to use perfect forwarding
        // auto task = std::bind(std::forward<Func>(func),std::forward<Args>(args)...);

        // Since we want the threadpool to run any function with any return type, the decltype() will deduce the return type
        using return_type = decltype(func(args...));
        // to add support for tasks to return a value, use packaged_task to get the std::future object
        // std::packaged_task<return_type()> pkg_task(std::bind(std::forward<Func>(func),std::forward<Args>(args)...));

        // since the packaged_task is non-copyable, we cannot pass it as parameter, so we use std::move
        //  We want the packaged_task object to persist in the queue even this function ends, so we encapsulate it with smart_ptr
        // auto encapsulated_pkTask = std::make_shared<std::packaged_task<return_type()>>(std::move(pkg_task));

        // OR We can use this to construct packaged task in-place
        auto task = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
        auto survivorPtr_pkTask = std::make_shared<std::packaged_task<return_type()>>(std::move(task));

        std::future<return_type> result = survivorPtr_pkTask->get_future();

        std::unique_lock<std::mutex> mLock(mtx);
        // Wait until there is space in the queue
        if (!cond_.wait_for(mLock, std::chrono::milliseconds(20), [this]()
                            { return (task_queue_.size() < capacity) || stop_requested_.load(std::memory_order_acquire); }))
            throw std::runtime_error("Timeout! Queue is full.");

        // If pool is stopped, do no not push any tasks to the queue
        // TODO: implement stop condition
        if (stop_requested_.load(std::memory_order_acquire))
            throw std::runtime_error("Cannot enqueue new tasks as thread pool is stopped");
        //task_queue_.emplace([survivorPtr_pkTask]()
        //                 { (*survivorPtr_pkTask)(); });
        mLock.unlock();
        cond_.notify_one();
        return result;
    }
    
    */
};



template<typename Task>
ThreadPool<Task>::ThreadPool(std::size_t task_capacity, std::size_t max_workers) :
					maxWorkers(max_workers), 
					task_queue_(task_capacity), 
					work_items(0),
					submitted(0),
					consumed(0)
{
    completed_tasks = 0;
    stop_requested_.store(false, std::memory_order_release);
    
    // Initialize worker threads
    for (size_t i = 0; i < maxWorkers; i++)
        worker_threads.emplace_back([this](){ this->startWorkerThread(); }); 
                                 // lamba fn to pop a task from queue to execute
}


template<typename Task>
bool ThreadPool<Task>::taskSubmit(Task& task)
{
    if ( stop_requested_.load(std::memory_order_acquire) )
        return false;
    
    if (task_queue_.tryPush(task) )
    {
    	work_items.release();
    	submitted++;
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
    	submitted++;
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
		submitted++;
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
    
    std::cout << "Starting thread: " << std::hex << std::this_thread::get_id() << std::endl;
    
    // keep polling for new tasks on this thread
    while (1)
    {
        // Wait until there is a task in the queue
		work_items.acquire();
		consumed++;

		// return only if pool is stopped and all tasks are completed
		if (stop_requested_.load(std::memory_order_acquire))
		{
			if(task_queue_.empty())
				goto stop_worker;		
		}
			
        // Pop a task from the queue to attach to current worker thread
        if ( !task_queue_.tryPop(task) )
        {
        	/*
       		 std::cout << "tryPop failed. stop_pool = "
				      << stop_requested_.load()
				      << ", queue empty = "
				      << task_queue_.empty()
				      << " thread = " << std::this_thread::get_id() << std::endl;
			*/
			
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
	std::cout << "Exit thread: " << std::hex << std::this_thread::get_id() << std::endl;
	return;
	
}




#endif /* SIMPLE_THREADPOOL_H */
