Trying to translate "design_Component_responsibilites" into an interface: 

Units: 
Task -> one unit of work  (using template WorkType)

## Main threadpool system: Threadpool<WorkType,WorkCoordinator>( task capacity, max workers)

What does threadpool do ? (member functions)
	- Lifecycle: 
				1. launchWorkers()
				2. requestStop() -> void
				3. stop_requested_
	- Task Execution: 
				1. taskSubmit(Tasks) -> Accepted/Rejected
				2. acquireWorkBlocking(work_buffer) -> (bool) (work_count==0)?Drained:WorkAvailable 
				3. runWorker()  worker loop 
				4. Track work execution/completion stats
				 
For these responsibilites, the threadpool should own : (member variables)
	- pool_state_
	- stop_requested_
	- Worker_Threads[N]
	- TaskAccountingStata
	- WorkCoordinator\< WorkType, (conceptual) QUeueContainer \>

### INVARAINTS:
	- WorkCoordinator MUST outlive the lifetime of every worker thread. Since worker threads are owned by Threadpool, but worker coordination responsibility is with WorkCoordinator.
	- WorkCoordinator only borrow access to Workers, and doesn not own them.
	- in acquireWork() blocking, the returned count should represent - count=0 only when coordinator guarantees work is drained for shutdown request, count>0 means work is available 
	- "stop_requested_" flag must act as a gate for new work admission, it must stop new work from being submitted if threadpool lifecycle state is STOPPING/STOPPED.
	- runWorker() is the worker loop per worker. It must acquire work to execute and block until new work is available. runWorker() does not depend on "stop_requested_" directly, instead it uses acquireWorkBlocking() return value as the ultimate source of thruth of lifecycle state. If acquireWorkBlocking() returns count=0, the runWorker() must interpret as pool has shutdown and remaining tasks are drained, so retire worker safely. 
	- ThreadPool never directly touches semaphores/CVs/atomic wait state used by WorkCoordinator for signalling/Coordination.
	
## WorkCoordinator: WorkCoordinator<WorkType>(task capacity, max workers)

What should the Work coordinator do : (candidates for member functions)

	- Work submission and distribution: 
				1. submit(Task) -> Accepted/Rejected
				2. Decides where and how received work is stored
				3. owns routing work policy
				
	- Work acquisition: 
				1. Determines where and how a worker can obtain executable work
				2. acquireWorkBlocking( Worker[id], work_buffer ) -> (size_t) work_count
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

Coordinator contract:

submit(task)
    -> Accepted / Rejected
    -> does NOT decide pool lifecycle admission

acquireWorkBlocking(worker_id, out_task)
    -> 1 = WorkAvailable
    -> 0 = Drained
    -> does NOT expose temporary emptiness

beginDrain()
    -> one-time transition from ThreadPool
    -> Guarantee from Threadpool of no new work will arrive
    -> wake blocked acquisition paths as needed
    -> preserve accepted work
    
Now trying to come up with a way to refine and split WorkCoordinator responsibilities also, otherwise it is like moving all old threadpool code in Coordinator class, without any added architectureal improvements. 
Currently WorkCoordinator has possibly these responsibilies for WOrk: 
	- Storage 
	- Routing 
	- Acquizition
	- Coordination/Signalling

Maybe I can try to split it further and pick QueueContainer and see which responsibilities can be delegated to it.

### Invariants: 

	- QueueContainer exposes a concurrency capability. WorkCoordinator must never exceed that capability without explicitly adapting the access pattern.
	- (Debatable )Possibly WorkCoordinator owns the signalling policy/semantics; the concrete work-source/queue wrapper may own the signalling primitive.
	- WorkCoordinator never directly manipulates queue storage internals.
	
## QueueContainer: QueueContainer<WorkType>(capacity)

Initial thought-  It must own:
					1. Raw Storage container Queue
					2. Required synchronization for Queue access
	For Example:				
	SPSC queue container 
		- guarantees safe synchronization between single producer <=> single consumer, 
		- but if multiple producer/multiple conusmer , that need to be handled by Coordinator 
		
	SPMC queue container 
		- guarantees 1 producer multi- consumer synchronization , 
		- multi propducer case need to be synchronized externally by Coordinator etc 
		
	Sharded queue 
		- synchronization guarantee depends on internal per queue type, 
		- anything beyond this to be synchronized by Coordinator

CORE IDEA:
A queue/container should own the synchronization required to make its own documented concurrency contract correct. The WorkCoordinator is responsible for using that contract correctly at the system level.

### Invariants: 

	- QueueContainer exposes a concurrency capability. WorkCoordinator must never exceed that capability without explicitly adapting the access pattern.
