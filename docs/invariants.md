# Design Invariants


## ThreadPool
	- ThreadPool owns worker threads lifecycle
	- ThreadPool owns task scheduling and queue<->worker synchronization
	- ThreadPool never manipulates task queue internals directly.
	- No popped task is executed more than once
	- No new task is accepted after shutdown is requested

## Task Queue
	- Owns all synchronization for concurrent pushes and pops to the queue container.
	- Owns queue capacity.
	- Has no knowledge of worker threads, or signalling mechanism between queue and threads.

## Synchronization
	- Number of available tasks decide how many threads can execute without waiting using a semaphore
	- threads must wait for new tasks if queue is empty
	- After every new task submitted, threadpool must issue semaphore signal operation
	- Before every attempt to consumer a task from the queue, threadpool must issue a wait 

## Shutdown Mechanism
	- Shutdown must be graceful, the queue should be flushed before all threads terminate
	- Desctructor must join every worker thread
