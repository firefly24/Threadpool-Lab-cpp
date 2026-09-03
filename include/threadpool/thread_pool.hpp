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
#include <memory>

//#include "queue/concurrent_queue.hpp"
#include "./instrumentation/tracing.hpp"
//#include "queue/SPMC_locked/spmc_queue.hpp"
#include "queue/SPSC_queue/spsc_lockfree.hpp"

static constexpr std::size_t CREATED = 0;
static constexpr std::size_t RUNNING = 1;
static constexpr std::size_t STOPPED = 2;

template <typename Task>
class ThreadPool
{
private:
    std::size_t pool_capacity_;
    std::size_t max_workers_;
    
    // per-worker queue specific 
    QueueTopology queue_type_;
  	std::vector<std::unique_ptr<SPSCQueue<Task>>> worker_queues_;
    std::vector<std::unique_ptr<std::counting_semaphore<INT_MAX>>> work_;
    std::size_t next_worker_;
    
    
    // thread pool state related
    std::atomic<unsigned int> completed_tasks_;
    std::atomic<bool> stop_requested_;
    std::atomic<bool> threadpool_stopped_;
    std::atomic<std::size_t> pool_state_;
     
    std::vector<std::thread> worker_threads_;
   
    // Disable copying
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    // Disable moving
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    // Worker thread function to pop tasks from the queue and execute them
    void startWorkerThread(std::size_t worker_id) noexcept;

public:
						
	// initialize threadpool, for per worker behavior
	explicit ThreadPool(std::size_t task_capacity, 
						std::size_t max_workers);

    unsigned int completedTaskCount()
    {
        return completed_tasks_.load(std::memory_order_relaxed);
    }
    
    // External interface to Launch all worker threads manually, can be launched only once in a threadpool object lifetime
    bool launchWorkers();
    
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
ThreadPool<Task>::ThreadPool(std::size_t task_capacity, 
							 std::size_t max_worker) :
					pool_capacity_(task_capacity),
					max_workers_(max_worker),  
					queue_type_(QueueTopology::PerWorker),
					next_worker_(0),
					completed_tasks_(0),
					stop_requested_(false),
					threadpool_stopped_(false),
					pool_state_(CREATED)			
{

	// For now, if queue type is mentioned, this constructor assumes Per Worker sharded queues by default
	std::size_t perworker_capacity = pool_capacity_/max_workers_;
	std::size_t leftover = pool_capacity_ % max_workers_;
	
	worker_queues_.reserve(max_workers_);	
	work_.reserve(max_workers_);	
	
	for (std::size_t worker = 0; worker < max_workers_; worker++)
	{
		std::size_t per_queue_capacity = perworker_capacity 
									+ ((worker < leftover)?1:0);
		worker_queues_.push_back(std::make_unique<SPSCQueue<Task>>(per_queue_capacity));
		work_.push_back(std::make_unique<std::counting_semaphore<INT_MAX>>(0));
	}
}


template<typename Task>
bool ThreadPool<Task>::launchWorkers()
{
	std::size_t expected = CREATED;
	if (!pool_state_.compare_exchange_strong(expected,RUNNING,std::memory_order_seq_cst))
			return false;
		
    // Initialize worker threads
    for (size_t worker = 0; worker < max_workers_; worker++) 
    {
        worker_threads_.emplace_back([this,worker]() 
        	{ 
        		std::string worker_name = "TPWorker-"+ std::to_string(worker);
    			pthread_setname_np(pthread_self(), worker_name.c_str());

        		this->startWorkerThread(worker);
        	}
        ); 
	}
	
	return true;
}


template<typename Task>
bool ThreadPool<Task>::taskSubmit(Task& task)
{
	
	std::size_t worker = next_worker_;
	
	next_worker_ = (next_worker_+1) % max_workers_;
	
	// possible race where task can still submit after stop requested
    if (!stop_requested_.load(std::memory_order_acquire)  )
    {
    	if (worker_queues_[worker]->tryPush(task))
 		{
    		work_[worker]->release();
    		return true;
    	}
    }
    	
    return false;
}


template<typename Task>
bool ThreadPool<Task>::taskSubmit(Task&& task)
{
	std::size_t worker = next_worker_;
	
	next_worker_ = (next_worker_ +1)% max_workers_;
	// possible race where task can still submit after stop requested
    if (!stop_requested_.load(std::memory_order_acquire) ) 
    {
    	if (worker_queues_[worker]->tryPush(std::move(task)) )
    	{
    		work_[worker]->release();
    		return true;
    	}
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
	
	if (pool_state_.load(std::memory_order_acquire) == RUNNING)
	{
		
		// Notify all worker threads to wake up and return now as queue is empty
		for (std::size_t worker=0; worker<max_workers_; worker++)
			work_[worker]->release();
			
		// Join all worker threads
		for (auto &worker : worker_threads_)
		{
			if (worker.joinable())
				worker.join();
		}
	}	
	
	threadpool_stopped_.store(true,std::memory_order_release);
	
	pool_state_.store(STOPPED,std::memory_order_release);
	
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
void ThreadPool<Task>::startWorkerThread(std::size_t worker_id) noexcept
{
    Task task;
    
    auto& task_queue_ = *worker_queues_[worker_id];
    auto& work_items_ = *work_[worker_id];
    
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


#endif /* SIMPLE_THREADPOOL_H */
