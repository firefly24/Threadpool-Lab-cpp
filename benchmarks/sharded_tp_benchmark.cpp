
#include <benchmark/benchmark.h>
#include <chrono>
#include <cstddef>

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
 *		Number of worker threads.
 *		batch size
 *		Size of task 	   		
 *
 * Fixed:
 *     Queue size
 *     Number of tasks
 *
 * Expected:
 *     Throughput initially increases with workers and eventually
 *     plateaus or decreases due to synchronization overhead.
 */
 
static void BM_TaskSchedSharded(benchmark::State& state, Task workload)
{
	std::string benchmarker_thread_name = "Bench-Main";
    pthread_setname_np(pthread_self(), benchmarker_thread_name.c_str());	
	
	// set-up the parameters
	std::size_t queue_size = state.range(0);
	std::size_t num_workers = state.range(1);
	std::size_t num_tasks = state.range(2);	
	
	
	
	// for stats and diagnoatics
	std::size_t total_tasks =0;
	std::size_t completed_tasks = 0;
	std::size_t accepted_tasks =0;
	std::size_t rejected_tasks = 0;
	
	// total_tasks = accepted + rejected
	// accepted = completed + dropped
	// ideally dropped ==0 once task submitted successfully
	
	std::chrono::nanoseconds total_producer_time{0};
	std::chrono::nanoseconds total_consumer_time{0};
	
	
	for (auto _ : state)
	{
		// Construct threadpool
		ThreadPool<Task> thread_pool(queue_size,num_workers);
		
		// start workers 
		thread_pool.launchWorkers();
		
		auto consumer_start = std::chrono::steady_clock::now();
		auto producer_start = std::chrono::steady_clock::now();
		{
			TP_TRACE_EVENT("ProducerBatchSubmit");
			for(std::size_t task=0; task<num_tasks ; task++)
			{
				// submit tasks
				if (!thread_pool.taskSubmit(workload))
					rejected_tasks++;
			}
		}
		auto producer_end = std::chrono::steady_clock::now();
		

		
		// destroy threadpool
		thread_pool.stopPool();
		
		auto consumer_end = std::chrono::steady_clock::now();
		
		// Calculate total producer time taken to submit tasks
		total_producer_time += std::chrono::duration_cast<std::chrono::nanoseconds>(producer_end-producer_start);
		
		// Calculate total consumer time taken to complete tasks
		total_consumer_time +=
		 std::chrono::duration_cast<std::chrono::nanoseconds>(consumer_end-consumer_start);
		
		total_tasks += num_tasks;
		
		// Update benchmark of completed tasks
		completed_tasks += thread_pool.completedTaskCount();
	}
	
	double producer_duration = std::chrono::duration<double>(total_producer_time).count();
	double consumer_duration = std::chrono::duration<double>(total_consumer_time).count();

	accepted_tasks = total_tasks - rejected_tasks;
	
	double producer_throughput = ((double)accepted_tasks/producer_duration); 
	double consumer_throughput = (static_cast<double>(completed_tasks)/consumer_duration); 
	
	state.SetItemsProcessed(completed_tasks);
	state.counters["producer_items_per_second"] = producer_throughput;
	state.counters["consumer_items_per_second"] = consumer_throughput;
	state.counters["Total tasks"] = total_tasks;
	
	state.counters["accepted tasks"] = accepted_tasks;
	state.counters["Completed tasks"] = completed_tasks;
	state.counters["Rejected tasks"] = rejected_tasks;
	
}

BENCHMARK_CAPTURE( BM_TaskSchedSharded,	// benchmarking function
					EmptyTask, 			// label for output
					Task(emptyTask)		// Workload for benchmarking
				)->ArgsProduct({
								{100000},
								{1,2,4,6,8},
								{100000},
							   })->UseRealTime();
 				
BENCHMARK_CAPTURE( BM_TaskSchedSharded,	// benchmarking function
					SmallTask, 			// label for output
					Task(smallTask)		// Workload for benchmarking
				)->ArgsProduct({
								{100000},
								{1,2,4,6,8},
								{100000},
							   })->UseRealTime();
			
BENCHMARK_CAPTURE( BM_TaskSchedSharded,	// benchmarking function
					MediumTask, 			// label for output
					Task(mediumTask)		// Workload for benchmarking
				)->ArgsProduct({
								{100000},
								{1,2,4,6,8},
								{100000},
							   })->UseRealTime();
							   
							   
// Takes quite some time, so run only when benchmarking for heavy workloads

BENCHMARK_CAPTURE( BM_TaskSchedSharded,	// benchmarking function
					LargeTask, 			// label for output
					Task(largeTask)		// Workload for benchmarking
				)->ArgsProduct({
								{100000},
								{1,2,4,6,8},
								{100000},
							   })->UseRealTime();

/**/

int main(int argc, char** argv)
{

	tracing::threadpool::initialize();
	
	::benchmark::Initialize(&argc,argv);
	
	if (::benchmark::ReportUnrecognizedArguments(argc,argv))
		return 1;
		
	::benchmark::RunSpecifiedBenchmarks();

	return 0;
}

//BENCHMARK_MAIN();
