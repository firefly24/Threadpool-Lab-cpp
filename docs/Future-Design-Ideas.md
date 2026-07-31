
##01/08/2026


### Idea
	Move synchronization responsibility out of threadpool, and let threadpool handle only scheduling.
	Let the synchronization be handled by the queue instead.
	I had this idea when I changed the threadpool's synchronization mechanism from semaphore based to condition_variable based and observed a higher thoughput. I would have liked to use interchangably while benchmakring and experiments. This would be possible if queue owned the synchronization instead.
	
	Benefits: 
		- Queues using different synchronization strateggies can be just plugged-in to the threadpool and tested independently. 
		- Can reuse threadpool implementation for different queue handling strategies
		- Worker thread implementation becomes simple and straightforward without worrying about handling synchronization for queue access
		
