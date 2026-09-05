Trying to translate "design_Component_responsibilites" into an interface: 

Units: 
Task -> one unit of work  (using template WorkType)

## Main threadpool system: Threadpool<WorkType,WorkCoordinator>( task capacity, max workers)

What does threadpool do ? (member functions)
	- Lifecycle: 
				1. launchWorkers()
				2. requestStop() -> void
				3. Owns worker lifetime
	- Task Execution: 
				1. taskSubmit(Tasks) -> Accepted/Rejected
				2. acquireWorkBlocking(work_buffer) -> (bool) (work_count==0)?Drained:WorkAvailable 
				3. Execute acquired tasks
				4. Track work execution/completion stats
				 
For these responsibilites, the threadpool should own : (member variables)
	- Lifecyclestate
	- Worker_Threads[N]
	- TaskAccountingStata
	- WorkCoordinator\< WorkType, (conceptual) QUeueContainer \>

### INVARAINTS:
	- WorkCoordinator MUST outlive the lifetime of every worker thread. Since worker threads are owned by Threadpool, but worker coordination responsibility is with WorkCoordinator.
	- WorkCoordinator only borrow access to Workers, and doesn not own them.
	- in acquireWork() blocking, the returned count should represent - count=0 only when coordinator guarantees work is drained for shutdown request, count>0 means work is available 
	 
	
## WorkCoordinator: WorkCoordinator<WorkType>(task capacity, max workers)

What should the Work coordinator do : (candidates for member functions)

	- Work submission and distribution: 
				1. submit(Task) -> Accepted/Rejected
				2. Decides where and how received work is stored
				3. owns routing work policy
				
	- Work acquisition: 
				1. Determines where and how a worker can obtain executable work
				2. acquireWork( Worker[id], work_buffer ) -> (size_t) work_count
				3. owns waiting/blocking for newly available work 
				4. owns worker<->queue relation coordination contract
				
	- Shutdown coordination:
				1. handleShutdown() -> void
				2. Facilitates graceful shutdown, incluing waking any blocked workers, 
				3. preserves queued work, and allow accepted work to drain
				4. Informs new work acquisition request that no more work remaining

For these responsibilites, WorkCoodrinator should own : 
	- QueueContainer \< Task, topology \> (can encapsulate one or more queues)
	- QueueTopology (conceptual for now, may be encoded later in container)
	- task routing state
	- signalling mechanism for wait/wake workers
	- work distribution handling related states
	

