#include <cassert>
#include <iostream>
#include <cstddef>
#include <thread>

#include "queue/SPSC_queue/spsc_lockfree.hpp"

//template <typename T>

void producerFn(SPSCQueue<int>& q,int num_items)
{
	assert(num_items > 0);
	int push_fails = 0;
	
	for(int item = 0;item < num_items; item++)
	{
		while (!q.tryPush(item))
			push_fails++;
		
	}
	//while(!q.tryPush(l))

}

void consumerFn(SPSCQueue<int>& q,int num_items)
{
	int expected = 0;
	
	int value;
	while( expected < num_items )
	{
		if (q.tryPop(value))
		{
			assert(value == expected);
			expected++;
		}
	}
}

void spscConcurrent(int queue_size, int num_items)
{
	assert(queue_size >0);
	
	//int num_items = queue_size;
	//assert(num_items > 0);
	
	SPSCQueue<int> q(queue_size);
	
	std::thread producer(producerFn, std::ref(q), num_items);
	std::thread consumer(consumerFn, std::ref(q), num_items);
	
	producer.join();
	consumer.join();

	std::cout << "Done test: queuesize: " << queue_size << std::endl;
}


void pushUntilFull(int queue_size)
{
	assert(queue_size > 0);
	int expected_failures = 10;
	int items = queue_size + expected_failures;
	
	int observed_failures =0;
	int observed_success = 0;
	
	SPSCQueue<int> q(queue_size);
	
	for (int item = 0; item<items;item++)
	{
		if (!q.tryPush(item))
			observed_failures++;
		else
			observed_success++;
	}
	assert(expected_failures == observed_failures);
	assert(observed_success == queue_size);
	
	int pop_failures = 0;
	int value;
	for (int item = 0; item < queue_size; item++)
	{
		if(!q.tryPop(value))
			pop_failures++;
	}
	assert(!pop_failures);
	assert(!q.tryPop(value));
	assert(q.empty());
	assert(q.tryPush(42));
	assert(!q.empty());
}




int main()
{
	int queue_size = 100000;
	int numtasks = 10000000;
	
	pushUntilFull(queue_size);
	spscConcurrent(queue_size,queue_size);
	
	spscConcurrent(1,numtasks);
	spscConcurrent(2,numtasks);
	spscConcurrent(3,numtasks);
	spscConcurrent(7,numtasks);
	spscConcurrent(8,numtasks);
	spscConcurrent(31,numtasks);

	return 0;
}

