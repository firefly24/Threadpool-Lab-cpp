Separate Architecture boundaries, and redesign components of the threadpool system. Each componenet should have clear defined boundaries of responsibilities: 


## Main threadpool system: 

What does threadpool do ? (member functions)
	- Lifecycle: 
				1. Decides when Worker threads are launched
				2. Decices and orders for graceful shutdown of the worker system
				3. Owns worker lifetime
	- Task Execution: 
				1. provides entry point for task submission for the pool
				2. Requests Work coordinator for runnable work
				3. Execute acquired tasks
				4. Track work execution/completion stats
				 
For these responsibilites, the threadpool should own : (member variables)
	- Lifecycle state
	- WOrker thread ownership
	- Work Execution and Accounting stats
	- Work Coordinator
	
## WorkCoordinator: 

What should the Work coordinator do : (candidates for member functions)

	- Work submission and distribution: 
				1. Accepts work passed by threadpool
				2. Decides where and how received work is stored
				3. owns routing work policy
				
	- Work acquisition: 
				1. Determines where and how a worker can obtain executable work
				2. owns retieval of available work
				3. owns waiting/blocking for newly available work 
				4. owns worker<->queue relation coordination contract
				
	- Shutdown coordination:
				1. Receives graceful shutdown request from threadpool
				2. Facilitates graceful shutdown, incluing waking any blocked workers, 
				3. preserves queued work, and allow accepted work to drain
				4. Informs new work acquisition request that no more work remaining

For these responsibilites, WorkCoodrinator should own : 
	- Task/Work container (storage  queue)
	- Queue container topology
	- task routing state
	- signalling mechanism for wait/wake workers
	- work distribution handling related states
	

