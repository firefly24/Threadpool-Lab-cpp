
#include <benchmark/benchmark.h>
#include <chrono>
#include <iostream>

#include "tasks.hpp"
#include "threadpool/instrumentation/tracing.hpp"


#include "threadpool/thread_pool.hpp"

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
	std::string benchmarker_thread_name = "Bench-Main";
    pthread_setname_np(pthread_self(), benchmarker_thread_name.c_str());	
	
	// set-up the parameters
	int queue_size = state.range(0);
	int num_workers = state.range(1);
	int num_tasks = state.range(2);	
	
	int total_completed_tasks= 0;
	
	std::chrono::nanoseconds total_producer_time{0};
	//double total_producer_time_ms =0;
		
	int total_tasks =0.0;
	
	for (auto _ : state)
	{
		// Construct threadpool
		ThreadPool<Task> thread_pool(queue_size,num_workers);
		
		auto start = std::chrono::steady_clock::now();
		{
			TP_TRACE_EVENT("ProducerBatchSubmit");
			for(int task=0; task<num_tasks ; task++)
			{
				// submit tasks
				thread_pool.taskSubmit(workload);
			}
		}
		auto end = std::chrono::steady_clock::now();
		// destroy threadpool
		thread_pool.stopPool();
		
		//total_producer_time_ms += std::chrono::duration<double, std::milli>(end - start).count();
		total_producer_time += std::chrono::duration_cast<std::chrono::nanoseconds>(end-start);
		total_tasks += num_tasks;
		
		// Update benchmark of completed tasks
		total_completed_tasks += thread_pool.completedTaskCount();
	}
	
	double producer_duration = std::chrono::duration<double>(total_producer_time).count();
	
	std::cout << "Total tasks: " << total_tasks << std::endl;
	std::cout << "Completed tasks: " << total_completed_tasks << std::endl;
	std::cout << "Producer throughput: " 
		<< ((double)total_tasks/producer_duration/1000.0) <<"k/s" << std::endl;
	std::cout << "Producer duration: " << producer_duration << std::endl;
	
	state.SetItemsProcessed(total_completed_tasks);
	
}

BENCHMARK_CAPTURE( BM_TaskScheduling,	// benchmarking function
					EmptyTask, 			// label for output
					Task(emptyTask)		// Workload for benchmarking
				)->ArgsProduct({
								{100000},
								{1,2,3,4,5,6,8},
								{100000}
							   })->UseRealTime();
 				
BENCHMARK_CAPTURE( BM_TaskScheduling,	// benchmarking function
					SmallTask, 			// label for output
					Task(smallTask)		// Workload for benchmarking
				)->ArgsProduct({
								{100000},
								{1,2,3,4,5,6,8},
								{100000}
							   })->UseRealTime();
			
BENCHMARK_CAPTURE( BM_TaskScheduling,	// benchmarking function
					MediumTask, 			// label for output
					Task(mediumTask)		// Workload for benchmarking
				)->ArgsProduct({
								{100000},
								{1,2,3,4,5,6,8},
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

int main(int argc, char** argv)
{

	tracing::threadpool::initialize();
	
	::benchmark::Initialize(&argc,argv);
	
	if (::benchmark::ReportUnrecognizedArguments(argc,argv))
		return 1;
		
	::benchmark::RunSpecifiedBenchmarks();
	
	//std::this_thread::sleep_for(std::chrono::seconds(10));

	return 0;
}

//BENCHMARK_MAIN();
