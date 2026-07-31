
#include <benchmark/benchmark.h>
#include "threadpool/thread_pool.hpp"

using Task = std::function<void()>;

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

}

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

BENCHMARK(BM_ThreadPoolExecution)->Args({100000,1,100000})->Args({100000,2,100000})->Args({100000,4,100000})->Args({100000,6,100000})->Args({100000,8,100000});



BENCHMARK_MAIN();
