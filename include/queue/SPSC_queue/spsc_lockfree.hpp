#pragma once

#include <vector>
#include <utility>

#include "../queue_helpers.hpp"

template <typename T>
class SPSCQueue
{
private:
    PaddedAtomicIdx head; // Index of first item
    PaddedAtomicIdx tail; // Index of the next empty slot after last item
    std::size_t capacity_;
    std::vector<T> ring_buff;
    
    std::size_t advance(std::size_t index) const;

public:
	
	explicit SPSCQueue(std::size_t capacity);

	// Disable copy 
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    
    // Disable move
    SPSCQueue(SPSCQueue&&) = delete;
    SPSCQueue& operator=(SPSCQueue&&) = delete;
    
    // Push item by Producer to the end of the queue
    bool tryPush(const T& item);
    
    // Push for move version
    bool tryPush(T&& item);
    
    // Remove/consume item by consumer thread from the front of the queue
    bool tryPop(T &out);

    bool empty() const;

};


template <typename T>
std::size_t SPSCQueue<T>::advance(std::size_t index) const
{
	// while advancing index, wrap around the ring buffer
    return (index+1)%capacity_;
}

template <typename T>
SPSCQueue<T>::SPSCQueue (std::size_t Capacity): head(0),
												tail(0),
												capacity_(Capacity+1),
												ring_buff(capacity_)
{
	static_assert(alignof(PaddedAtomicIdx) >= 64);
	static_assert(sizeof(PaddedAtomicIdx) >= 64);
}

template <typename T>
bool SPSCQueue<T>::tryPush(const T& item)
{
    std::size_t back = tail.index.load(std::memory_order_relaxed);
   	std::size_t next = advance(back);

    // Check if queue is full
    if (next == head.index.load(std::memory_order_acquire))
        return false;
           
    ring_buff[back] = item;
    
    // publish new tail
    tail.index.store(next,std::memory_order_release); 
    return true;
}

template <typename T>
bool SPSCQueue<T>::tryPush(T&& item)
{
    std::size_t back = tail.index.load(std::memory_order_relaxed);
    std::size_t next = advance(back);

    // Check if queue is full
    if (next == head.index.load(std::memory_order_acquire))
        return false;
    
    ring_buff[back] = std::move(item);

	// publish new tail
    tail.index.store(next,std::memory_order_release);  
    return true;
}

template <typename T>
bool SPSCQueue<T>::tryPop(T &out)
{    
    std::size_t front = head.index.load(std::memory_order_relaxed);
    
    // return if queue is empty
    if (front == tail.index.load(std::memory_order_acquire))
        return false;

    out = std::move(ring_buff[front]);

	// publish new head
    head.index.store(advance(front), std::memory_order_release);
    
    return true;
}


template <typename T>
bool SPSCQueue<T>::empty() const 
{
    return (head.index.load(std::memory_order_acquire) 
    		== tail.index.load(std::memory_order_acquire));
}


