#include <chrono>
#include <iostream>
#include "threadpool/thread_pool.hpp"

using Task = std::function<void()>;

void run_benchmark(int queue_size, int numworkers, int numtasks)
{
	if (queue_size <=0 || numworkers <0 || numtasks <0)
		return;
	
	//std::chrono::duration<std::chrono::milliseconds> time_elapsed;
	//int time_taken=0;
	
	int tasks_accepted = 0;	
	
	auto start = std::chrono::steady_clock::now();
	{
		ThreadPool<Task> thread_pool(queue_size,numworkers);
		
		for(int task=0; task<numtasks ; task++)
		{
			if (thread_pool.taskSubmit([](){}) )
				tasks_accepted++;
		}
		
		//thread_pool.stopPool();
	}
	
	auto end = std::chrono::steady_clock::now();
	
	auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
	
	
	std::cout << "Queue size :  " << queue_size << std::endl
			 << "Workers :  " << numworkers << std::endl
			 << "Tasks : " << numtasks << std::endl
			 << "Completed : " << tasks_accepted << std::endl
			 << "Time elapsed : " << time_elapsed << " ms" << std::endl
			 << "Throughput :  " << (double)((double)tasks_accepted/time_elapsed) << " tasks/ms"
			  << std::endl;

}

int main()
{
	run_benchmark(1000,8,100000);
	
	return 0;

}
