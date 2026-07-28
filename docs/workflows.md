 (will beautify later)

## Task Submission 

	Producer entity 
		|
		v
	ThreadPool::taskSubmit(Task)
		|
		v
	taskQueue::Push(Task)
		|
		v
	Semaphone::release()
		|
		v
	Return

--- 

## Task Execution

	Worker thread
		|
		V
	Semaphore::acquire()
		|
		V
	taskQueue::Pop(Task)
		|
		V
	Execute task
		|
		V
	Update statistics
		|
		V
	move on to waiting for new task

---

## Graceful Shutdown

	StopPool()
		|
		V
	reject any new taskQueue::Push()
		|
		V
	worker threads drain remaining taskQueue
		|
		V
	worker threads terminate
		|
		V
	Destructor joins all worker threads
		
		
## Worker Thread lifecycle



