
Original repository : https://github.com/firefly24/Concurrent-Queues-Lab-cpp.git

/***
 *  Implementing a simple lock-free queue , for Single Producer Multi-Consumer use-case
 *  Assumptions: 
 *      1. The items in the queue will be homogenous
 *      2. The capacity of the queue is bounded, and is decided at the time of queue initialization
 *      3. The queue will be accessed by a single producer only and multiple consumer
 *  
 *  Noting down implementation ideas:
 *      1. Using a vector to simulate a circular ring buffer of size of queue capacity
 *      2. have 2 indices for queue access- head(front) and tail(back). Both should be atomic.
 *      3. Producer will insert the item at the tail index (back of queue)
 *         Consumer will pop an item from the head index (front of queue)
 *		4. Head will point to the first item at the front of the queue
 			Tail will point to empty slot just after the last element of the queue
 		5. For single producer, only use tail atomic update to publish new push 
 		6. For multi-consumer, use head_lock_ to synchronize concurrent pushes to queue head
 *
 *  Notes from resources on std::atomic for reference:
 *      * atomic is used in lock-free concurrency programming, also for non-blocking operations which are free from data-races 
 *        meaning thread-safety is ensured for shared variables in concurrent scenarios, without the use of explict locking like locks and mutexes
 *      * There is a probability that internally the atomic operations may be implementated with locks ( need to confirm )
 *      * To check if an atomic operation is truly lock-free, std::atomic::is_lock_free() can be used
 *      * key atomic operations to remember for now -> load(), store(), compare_and_exchange(), ++/-- etc
 *      * Need to pay special attention to memory ordering in the atomic operations
 *      * In short, by default, the order of execution is strictly same as the order of instruction written in code
 *      * std::memory_order_relaxed is where the compiler is free to do optimizations by changing the order of instructions
 *      * std::memory_
 * 
 *  Videoes I referenced for understanding std::atomic, memory ordering and memory barriers:
 *      https://youtu.be/ZQFzMfHIxng?si=OtfwP7XMSlboYxb2
 *      https://youtu.be/IE6EpkT7cJ4?si=3A85Lp0waSTTnEzK (more visual explanations)
 * 
 */
