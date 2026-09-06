#ifndef SIMPLE_THREADPOOL_H
#define SIMPLE_THREADPOOL_H


#include <thread>
#include <atomic>
#include <iostream>
#include <vector>
#include <climits>
#include <cassert>
#include <utility>
#include <pthread.h>
#include <string>
#include <cstddef>
#include <memory>

#include "./instrumentation/tracing.hpp"
#include "coordinator/work_coordinator.hpp"

enum class PoolState{
	CREATED,
	//INITIALIZE,
	RUNNING,
	STOPPING,
	STOPPED
};

template <typename Task, typename Coordinator>
class ThreadPool
{
private:

	// Total number of workers 
    std::size_t max_workers_;
    
    Coordinator coordinator_;
    
    // thread pool lifecycle state related
    std::atomic<PoolState> pool_state_;
    std::atomic<bool> stop_requested_;
     
    // Accounting stats 
    std::atomic<unsigned int> completed_tasks_;
    
    // Worker threads
    std::vector<std::thread> worker_threads_;
   
    // Disable copying
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    // Disable moving
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    // Worker thread function to pop tasks from the queue and execute them
    void runWorker(std::size_t worker_id) noexcept;

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
    
    
    /** API contracts:
    *
    * Constructor-> launchWorkers -> stopPool ->Desctructor must be called on same thread,
    * concurrency between these functions is not tolerated, 
    * as they represent sequential lifecycle of the threadpool
    *
    * taskSumbit <-> stopPool concurrency is currently unsafe
    * Only APIs expected to run concurrently - taskSubmit and startWorkerThread
    * 
    */ 
};


template<typename Task, typename Coordinator>
ThreadPool<Task,Coodinator>::ThreadPool(std::size_t task_capacity, 
										std::size_t max_workers) :
												max_workers_(max_workers),
												coordinator_(task_capacity,max_workers),
												pool_state_( PoolState::CREATED ),
												stop_requested_(false),
												completed_tasks_(0)
{

	// TODO: Initializing any WorkCoordinator related mechanisms
	
	
	
	//pool_state_.store(PoolState::INITIALIZED)
}


template<typename Task, typename Coordinator>
bool ThreadPool<Task,Coordinator>::launchWorkers()
{
	std::size_t expected = PoolState::CREATED;
	
	if (!pool_state_.compare_exchange_strong(expected,
											 PoolState::RUNNING /*PoolState::INITIALIZE*/,
											 std::memory_order_seq_cst) )
	{
			return false;
	}
		
    // Initialize worker threads
    for (size_t worker = 0; worker < max_workers_; worker++) 
    {
        worker_threads_.emplace_back([this,worker]() 
        	{ 
        		std::string worker_name = "TPWorker-"+ std::to_string(worker);
    			pthread_setname_np(pthread_self(), worker_name.c_str());

        		this->runWorker(worker);
        	}
        ); 
	}
	
	return true;
	
	//return pool_state_.compare_exchange_strong( expected, 
	//											PoolState::RUNNING, 
	//											std::memory_order_seq_cst);
}


template<typename Task, typename Coordinator>
bool ThreadPool<Task,Coordinator>::taskSubmit(Task& task)
{
    //if (stop_requested_.load(std::memory_order_acquire)  ) return false;

	return coordinator_.submit(task);
}


template<typename Task, typename Coordinator>
bool ThreadPool<Task,Coordinator>::taskSubmit(Task&& task)
{
    // if (stop_requested_.load(std::memory_order_acquire) )  return false;
    
	return coordinator_.submit(std::move(task));
}



// shutdown the threadpool gracefully on request, not expecting concurrency 
template<typename Task, typename Coordinator>
void ThreadPool<Task,Coordinator>::stopPool()
{		
	std::size_t state = pool_state_.load(std::memory_order_acquire);
	
	if (state == PoolState::STOPPED)
		return;
	
	// notify threadpool to stop accepting new work
	stop_requested_.store(true, std::memory_order_release);
	
	if (state == PoolState::CREATED)
	{
		// TODO: refine later
		//coordinator_.handleStop();
		pool_state_.store(STOPPED,std::memory_order_release);
		return;
	}
	
	//pool_state_.wait(PoolState::INITIALIZE, std::memory_order_acquire);

	std::size_t expected = PoolState::RUNNING;
	if(pool_state_.compare_exchange_strong(expected,PoolState::STOPPING, std::memory_order_seq_cst))
	{
		// handle all shutdown mechanics
		coordinator_.handleStop();
			
		// Join all worker threads
		for (auto &worker : worker_threads_)
		{
			if (worker.joinable())
				worker.join();
		}
		
		pool_state_.store(STOPPED,std::memory_order_release);
	}	
}

template<typename Task, typename Coordinator>
ThreadPool<Task,Coordinator>::~ThreadPool()
{	
	stopPool();
}

// Worker thread function to pop tasks from the queue and execute them
template<typename Task, typename Coordinator>
void ThreadPool<Task,Coordinator>::runWorker(std::size_t worker_id) noexcept
{
	// NOTE: currently valid for 1 task only, redesign later for mult-tasks
    Task work_buff;
    
    // keep polling for new tasks on this thread
    while (true)
    {
    	// blocks until coordinator returns work
    	std::size_t task_count = coordinator_.acquireWorkBlocking(worker_id, &work_buff );
		
		// count = 0 iff threadpool has requested shutdown and all queues work has drained
		if (!task_count)
			goto stop_worker;
			
		while(task_count--)
		{
			work_buff();
			completed_tasks_.fetch_add(1,std::memory_order_relaxed);
		}
    }
stop_worker:
	//I want to retain this label as a single point in case any post exit cleanup needed later
	return;
	
}

/*

template<typename Task>
ThreadPool<Task>::ThreadPool(std::size_t task_capacity, 
							 std::size_t max_worker) :
					pool_capacity_(task_capacity),
					max_workers_(max_worker),  
					queue_type_(QueueTopology::PerWorker),
					next_worker_(0),
					completed_tasks_(0),
					stop_requested_(false),
					pool_state_(CREATED)			
{

	// For now, if queue type is mentioned, this constructor assumes Per Worker sharded queues by default
	std::size_t perworker_capacity = pool_capacity_/max_workers_;
	std::size_t leftover = pool_capacity_ % max_workers_;
	
	worker_queues_.reserve(max_workers_);	
	
	for (std::size_t worker = 0; worker < max_workers_; worker++)
	{
		std::size_t per_queue_capacity = perworker_capacity 
									+ ((worker < leftover)?1:0);
		worker_queues_.push_back(std::make_unique<NotifSPSCQueue<Task>>(per_queue_capacity));
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
    	if (!worker_queues_[worker]->tryPush(task))	
  		{
   			//next_worker_ = (next_worker_+1) % max_workers_;
   			return false;
   		}
   		return true;
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
    	if (!worker_queues_[worker]->tryPush(std::move(task)) )
    	{
    		//next_worker_ = (next_worker_+1) % max_workers_;
   			return false;
    	}
    	return true;
    }
    
	return false;
}

// shutdown the threadpool gracefully on request, not expecting concurrency 
template<typename Task>
void ThreadPool<Task>::stopPool()
{		
	std::size_t state = pool_state_.load(std::memory_order_acquire);
	
	if (state == STOPPED)
		return;
	
	// notify threadpool to stop accepting new work
	stop_requested_.store(true, std::memory_order_release);
	
	if (state == CREATED)
	{
		pool_state_.store(STOPPED,std::memory_order_release);
		return;
	}

	std::size_t expected = RUNNING;
	if(pool_state_.compare_exchange_strong(expected,STOPPING, std::memory_order_seq_cst))
	{
		
		// Notify all worker threads to wake up and return now as queue is empty
		for (std::size_t worker_id=0; worker_id<max_workers_; worker_id++)
			worker_queues_[worker_id]->wakeConsumer();
			
		// Join all worker threads
		for (auto &worker : worker_threads_)
		{
			if (worker.joinable())
				worker.join();
		}
		
		pool_state_.store(STOPPED,std::memory_order_release);
	}	
}


template<typename Task>
ThreadPool<Task>::~ThreadPool()
{	
	stopPool();
}


// Worker thread function to pop tasks from the queue and execute them
template<typename Task>
void ThreadPool<Task>::startWorkerThread(std::size_t worker_id) noexcept
{
    Task task;
    
    auto& local_queue_ = *worker_queues_[worker_id];
    
    // keep polling for new tasks on this thread
    while (true)
    {
		// return only if pool is stopped and all tasks are completed
		if (stop_requested_.load(std::memory_order_acquire))
		{
			if(local_queue_.empty())
				goto stop_worker;		
		}
			
        // Pop a task from the queue to attach to current worker thread
        if ( !local_queue_.tryPop(task) )
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

*/


#endif /* SIMPLE_THREADPOOL_H */
