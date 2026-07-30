#include <cassert>
#include <atomic>
#include <iostream>
#include "threadpool/thread_pool.hpp"

using Task = std::function<void()>;


/** Test: Basic task execution 
	Verifies - every accepted task executes exactly once
			 -	pool's completedTasks count matches accepted task count
			 - no task is lost between accepted and execution

*/
void testBasicExecution(int queue_size, int num_workers, int numtasks)
{
	assert(queue_size > 0 );
	std::atomic<int> submitted{0};
	std::atomic<int> tasks_executed{0};
	//int tasks_executed =0;
	
	// force threadpool lifetime complete before assert, otherwise assert might be fired before all thread complete and join
	{
		ThreadPool<Task> thread_pool(queue_size,num_workers);

		for (int task =0; task<numtasks; task++)
		{
			if ( thread_pool.taskSubmit([&tasks_executed](){ tasks_executed++; }))
				submitted++;
		}
	}
	assert(submitted <= numtasks);
	assert(submitted == tasks_executed);
}


/** Test: Queue capacity 
	Verifies - Threadpool never accepts more tasks then queue capacity
			 - Rejected tasks are never counted as part of completed tasks
*/
void testQueueCapacity(int queue_size, int numtasks)
{
	assert(queue_size > 0 );
	std::atomic<int> accepted{0};
	std::atomic<int> tasks_executed{0};
	std::atomic<int> tasks_rejected{0};
	
	const int num_workers = 0;
	
	// force threadpool lifetime complete before assert, otherwise assert might be fired before all thread complete and join
	{
		ThreadPool<Task> thread_pool(queue_size,num_workers);

		for (int task =0; task<numtasks; task++)
		{
			if ( thread_pool.taskSubmit([&tasks_executed](){ tasks_executed++; }))
				accepted++;
			else
				tasks_rejected++;
		}
	}
	assert(accepted == std::min(queue_size,numtasks));
	assert(tasks_rejected == (numtasks - accepted));
}


/** Test: Reject after shutdown
	Verifires - no new task is accepted after stopPool() is called
			  - taskSubmit() should consistently return false
*/
void testRejectAfterShutdown (int queue_size, int num_workers, int numtasks)
{
	assert(queue_size > 0 && numtasks >=0 && num_workers >=0);
	std::atomic<int> submitted{0};
	std::atomic<int> tasks_rejected{0};
	std::atomic<int> tasks_accepted{0};
	
	{
		ThreadPool<Task> thread_pool(queue_size, num_workers);
		
		thread_pool.stopPool();
		
		for (int task =0; task<numtasks; task++)
		{
			if ( thread_pool.taskSubmit([&submitted](){ submitted++; }))
				tasks_accepted++;
			else
				tasks_rejected++;
		}
	}
	
	assert(tasks_accepted ==0);
	assert(tasks_rejected == numtasks);
}	


/** Test: Multiple producers
	Verifies - every accepted task executes exactly once
			 -	pool's completedTasks count matches accepted task count
			 - no task is lost between accepted and execution

*/
void testMultipleProducers(int num_workers, int tasks_per_producer, int producer_count)
{
	assert(num_workers>=0 && tasks_per_producer>=0 && producer_count>=0);
	
	int queue_size = tasks_per_producer*(producer_count+1);
	
	std::atomic<int> tasks_executed{0};
	std::atomic<int> tasks_accepted{0};
	
	auto task_func = [&tasks_executed](){ tasks_executed++; };
	
	std::vector<std::thread> producers;
	
	{
		ThreadPool<Task> thread_pool(queue_size, num_workers);
		
		auto producer_job = [&](){
									for (int task =0; task<tasks_per_producer; task++)
									{
										if ( thread_pool.taskSubmit(task_func))
											tasks_accepted++;
									}
							 	};
		
		for (int producer=0; producer<producer_count; producer++)
		{
			producers.emplace_back(	producer_job);
		}
		
		for(auto& producer: producers)
			producer.join();
	}
	
	std::cout << "accepted: " << tasks_accepted << std::endl;
	std::cout << "executed: " << tasks_executed << std::endl;
	
	assert(tasks_accepted == (tasks_per_producer* producer_count));
	assert(tasks_executed == tasks_accepted);

}


/** Test: Multiple workers
	Verifies - every accepted task executes exactly once
			 -	pool's completedTasks count matches accepted task count
			 - no task is lost between accepted and execution

*/
void testMultipleWorkers(int num_workers, int num_tasks)
{
	assert(num_workers>=0 && num_tasks>=0);
	
	int queue_size = num_tasks;
	
	std::atomic<int> tasks_executed{0};
	std::atomic<int> tasks_accepted{0};
	
	auto task_func = [&tasks_executed](){ tasks_executed++; };
	
	{
		ThreadPool<Task> thread_pool(queue_size, num_workers);
		
		for (int task =0; task<num_tasks; task++)
		{
			if ( thread_pool.taskSubmit(task_func))
				tasks_accepted++;
		}
	}
	
	std::cout << "accepted: " << tasks_accepted << std::endl;
	std::cout << "executed: " << tasks_executed << std::endl;
	
	assert(tasks_accepted == num_tasks);
	assert(tasks_executed == tasks_accepted);

}


/** Test: Graceful shutdown
	Verifires - no new task is accepted after stopPool() is called
			  - taskSubmit() should consistently return false
*/
void testGracefulShutdown (int queue_size, int num_workers, int numtasks)
{
	assert(queue_size > 0 && numtasks >=0 && num_workers >=0);
		
	std::atomic<int> tasks_executed{0};
	std::atomic<int> tasks_accepted{0};
	
	auto task_func = [&tasks_executed](){ 
											std::this_thread::sleep_for(
																std::chrono::milliseconds(20)
															);
											tasks_executed++; 
										};
	
	{
		ThreadPool<Task> thread_pool(queue_size, num_workers);
		
		std::thread producer([&](){
									for (int task =0; task<numtasks; task++)
									{
										if ( thread_pool.taskSubmit(task_func))
											tasks_accepted++;
											
										std::this_thread::sleep_for(
																std::chrono::milliseconds(5)
															);
									}
								}
							);
							
		std::this_thread::sleep_for(std::chrono::milliseconds(60));
		
		thread_pool.stopPool();

		producer.join();
	}
		
	std::cout << "accepted: " << tasks_accepted << std::endl;
	std::cout << "executed: " << tasks_executed << std::endl;
		
	assert(tasks_accepted == tasks_executed);
}	

int main()
{

	testBasicExecution(100,4,20);
	
	std::cout << "[PASS] Basic execution" << std::endl;
	
	testQueueCapacity(4,10);
	
	std::cout << "[PASS] Queue capacity" << std::endl;
	
	testRejectAfterShutdown(10,4,10);
	
	std::cout << "[PASS] Reject after shutdown" << std::endl;
	
	testMultipleProducers(8,100,4);
	
	std::cout << "[PASS] Multiple producers" << std::endl;
	
	testMultipleWorkers(10,100);
	
	std::cout << "[PASS] Multiple workers" << std::endl;
	
	testGracefulShutdown(100,4,60);
	
	std::cout << "[PASS] Graceful shutdown" << std::endl;
	
	return 0;
}
