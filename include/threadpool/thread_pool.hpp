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
#include <concepts>
#include <functional>

#include "./instrumentation/tracing.hpp"
#include "coordinator/sharded_work_coordinator.hpp"

enum class PoolState{
	CREATED,
	//INITIALIZE,
	RUNNING,
	STOPPING,
	STOPPED
};

template <typename T>
concept ExecutableTask = std::default_initializable<T>
					  && std::move_constructible<T>
					  && std::invocable<T>;

template <ExecutableTask Task, typename Coordinator>
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
    
    std::thread producer_;
   
    // Disable copying
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    // Disable moving
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;
    
    // Launch all worker threads, can be launched only once in a threadpool object lifetime
    bool launchWorkers();

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
    
    
    
    // lvalue overload- for Fire and forget tasks with no return value
    bool taskSubmit(Task& task);
    
    // rvalue overload- for Fire and forget tasks with no return value
    bool taskSubmit(Task&& task);
    
    bool taskSubmitBatch(std::vector<Task> tasks, std::size_t batch_size);
    
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
    * stop_requested_ : responsible for controlling admission of new work, 
    *					if threadpool requests shutdown, this flag must stop new submits
    *
    *
    *
    * 
    */ 
};


template<ExecutableTask Task, typename Coordinator>
ThreadPool<Task,Coordinator>::ThreadPool(std::size_t task_capacity, 
										std::size_t max_workers) :
												max_workers_(max_workers),
												coordinator_(task_capacity,max_workers),
												pool_state_( PoolState::CREATED ),
												stop_requested_(false),
												completed_tasks_(0)
{

	// TODO: Initializing any WorkCoordinator related mechanisms
	
	launchWorkers();
	
	//pool_state_.store(PoolState::INITIALIZED)
}


template<ExecutableTask Task, typename Coordinator>
bool ThreadPool<Task,Coordinator>::launchWorkers()
{
	enum PoolState expected = PoolState::CREATED;
	
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


template<ExecutableTask Task, typename Coordinator>
bool ThreadPool<Task,Coordinator>::taskSubmit(Task& task)
{
	if( stop_requested_.load(std::memory_order_acquire)  )
		return false;
		
	return coordinator_.submit(task);
}


template<ExecutableTask Task, typename Coordinator>
bool ThreadPool<Task,Coordinator>::taskSubmit(Task&& task)
{
	if( stop_requested_.load(std::memory_order_acquire) )  
		return false;
		
	return coordinator_.submit(std::move(task));
}

template<ExecutableTask Task, typename Coordinator>
bool ThreadPool<Task,Coordinator>::taskSubmitBatch(std::vector<Task> tasks, std::size_t batch_size)
{
	if( stop_requested_.load(std::memory_order_acquire) )  
		return false;
	
	return coordinator_.submitBatch(std::move(tasks),batch_size);		

}


// shutdown the threadpool gracefully on request, not expecting concurrency 
template<ExecutableTask Task, typename Coordinator>
void ThreadPool<Task,Coordinator>::stopPool()
{		
	enum PoolState state = pool_state_.load(std::memory_order_acquire);
	
	if (state == PoolState::STOPPED)
		return;
	
	// notify threadpool to stop accepting new work
	stop_requested_.store(true, std::memory_order_release);
	
	if (state == PoolState::CREATED)
	{
		// TODO: refine later
		//coordinator_.handleStop();
		pool_state_.store(PoolState::STOPPED,std::memory_order_release);
		return;
	}
	
	//pool_state_.wait(PoolState::INITIALIZE, std::memory_order_acquire);

	enum PoolState expected = PoolState::RUNNING;
	if(pool_state_.compare_exchange_strong(expected,PoolState::STOPPING, std::memory_order_seq_cst))
	{
		// handle all shutdown mechanics
		coordinator_.wakeAllWorkers();
			
		// Join all worker threads
		for (auto &worker : worker_threads_)
		{
			if (worker.joinable())
				worker.join();
		}
		
		pool_state_.store(PoolState::STOPPED,std::memory_order_release);
	}	
}

template<ExecutableTask Task, typename Coordinator>
ThreadPool<Task,Coordinator>::~ThreadPool()
{	
	stopPool();
}

// Worker thread function to pop tasks from the queue and execute them
template<ExecutableTask Task, typename Coordinator>
void ThreadPool<Task,Coordinator>::runWorker(std::size_t worker_id) noexcept
{
    
    // keep polling for new tasks on this thread
    while (true)
    {
    	// blocks until coordinator returns work
    	std::size_t task_count = coordinator_.acquireWorkBlocking(worker_id/*, work_buff */ );
		
		// count = 0 iff threadpool has requested shutdown and all queues work has drained
		if (!task_count)
			goto stop_worker;
    }
stop_worker:
	//I want to retain this label as a single point in case any post exit cleanup needed later
	return;
	
}

#endif /* SIMPLE_THREADPOOL_H */
