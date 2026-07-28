
# Thread Pool Architecture

## Overview: 

	Designed a threadpool in c++ which has a dedicated reusable pool of threads. It accepts work items from producer entities and facilitates work being picked and executed by the available thread pool.

## Components:
	 - ThreadPool
	 	- Tasks Queue
	 	- Worker Threads
	 	- synchronization mechanism
 	
## Responsibilities:

Task Queue - holds the submitted work items until it is picked up by a worker thread from the pool.
Worker Treads - Reusable thread that picks work item from task queue and executes/performs the task.
Synchronization - mechanism for task queue and worker threads to wait/ signal each other.

## Component interactions:

	Threadpool notifies the worker thread to wakeup to pickup new work when new task is pushed to the queue.


## Thread ownership:

	Threads in the pool are owned by the threadpool module, and not any external producer/consumer

## Synchronization primitives: 

	Using semaphores for now as a signalling mechanism for data availability between threads and queue.
