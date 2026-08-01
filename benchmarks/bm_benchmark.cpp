
#include <benchmark/benchmark.h>
#include "threadpool/thread_pool.hpp"
#include "tasks.hpp"


/**
 * Benchmark: any Task
 *
 * Purpose:
 *     Measure the throughput of the thread pool.
 *
 * Variable:
 *     Number of worker threads.
 *
 * Fixed:
 *     Queue size
 *     Number of tasks
 *     Task body (empty)
 *
 * Expected:
 *     Throughput initially increases with workers and eventually
 *     plateaus or decreases due to synchronization overhead.
 */
 
static void BM_TaskScheduling(benchmark::State& state, Task workload)
{
	// set-up the parameters
	int queue_size = state.range(0);
	int num_workers = state.range(1);
	int num_tasks = state.range(2);	
	
	int total_completed_tasks= 0;
		
	for (auto _ : state)
	{
		// Construct threadpool
		ThreadPool<Task> thread_pool(queue_size,num_workers);
		
		for(int task=0; task<num_tasks ; task++)
		{
			// submit tasks
			thread_pool.taskSubmit(workload);
		}
		
		// destroy threadpool
		thread_pool.stopPool();
		
		// Update benchmark of completed tasks
		total_completed_tasks += thread_pool.completedTaskCount();
	}
	
	state.SetItemsProcessed(total_completed_tasks);
	
}

BENCHMARK_CAPTURE( BM_TaskScheduling,	// benchmarking function
					EmptyTask, 			// label for output
					Task(emptyTask)		// Workload for benchmarking
				)->ArgsProduct({
								{100000},
								{1,2,4,6,8},
								{100000}
							   })->UseRealTime();
 				
BENCHMARK_CAPTURE( BM_TaskScheduling,	// benchmarking function
					SmallTask, 			// label for output
					Task(smallTask)		// Workload for benchmarking
				)->ArgsProduct({
								{100000},
								{1,2,4,6,8},
								{100000}
							   })->UseRealTime();
			
BENCHMARK_CAPTURE( BM_TaskScheduling,	// benchmarking function
					MediumTask, 			// label for output
					Task(mediumTask)		// Workload for benchmarking
				)->ArgsProduct({
								{100000},
								{1,2,4,6,8},
								{100000}
							   })->UseRealTime();
							   
							   
/** Takes quite some time, so run only when benchmarking for heavy workloads
BENCHMARK_CAPTURE( BM_TaskScheduling,	// benchmarking function
					LargeTask, 			// label for output
					Task(largeTask)		// Workload for benchmarking
				)->ArgsProduct({
								{100000},
								{1,2,4,6,8},
								{100000}
							   })->UseRealTime();

*/
//BENCHMARK(BM_EmptyTaskScheduling)->Args({100000,1,100000})->Args({100000,2,100000})->Args({100000,4,100000})->Args({100000,6,100000})->Args({100000,8,100000});

/*
static void BM_ThreadPoolExecution(benchmark::State& state)
{
	int queue_size = state.range(0);
	int numworkers = state.range(1);
	int numtasks = state.range(2);
	int accepted = 0;
	for( auto _: state)
	{
		ThreadPool<Task> thread_pool(queue_size,numworkers);
		
		for(int task=0; task<numtasks ; task++)
		{
			thread_pool.taskSubmit([](){
											volatile int x = 0;
											for (int i=0;i<1000; i++)
												x = x+1;
										});
		}
	
		thread_pool.stopPool();
		
		accepted += thread_pool.completedTaskCount();
		
	}
	
	state.SetItemsProcessed(accepted);

}*/

//BENCHMARK(BM_ThreadPoolExecution)->Args({100000,4,100000});

/*
static void BM_CheckSemaphore(benchmark::State& state)
{
	for( auto _: state)
	{
		std::counting_semaphore s{0};
		std::vector<std::thread> threads;
		for (size_t i = 0; i < 10; ++i) {
			threads.emplace_back([&s]() {
			  for (size_t i = 0; i < 1000000; ++i) {
				s.acquire();
			  }
			});
			threads.emplace_back([&s]() {
			  for (size_t i = 0; i < 1000000; ++i) {
				s.release();
			  }
			});
		}
		for (auto &t : threads) t.join();
	}
}

BENCHMARK(BM_CheckSemaphore);

*/

//BENCHMARK(BM_ThreadPoolExecution)->Args({100000,1,100000})->Args({100000,2,100000})->Args({100000,4,100000})->Args({100000,6,100000})->Args({100000,8,100000});



BENCHMARK_MAIN();
