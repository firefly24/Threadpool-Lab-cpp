#pragma once

#include <semaphore>
#include <climits>
#include <utility>
#include "spsc_lockfree.hpp"

template <typename T>
class NotifSPSCQueue {

private:

	// container
	SPSCQueue<T> queue_;
	std::size_t capacity_;
	
	// notification mechanism
	std::counting_semaphore<INT_MAX> has_work_;

public:
	explicit NotifSPSCQueue(std::size_t capacity);

	// Disable copy 
    NotifSPSCQueue(const NotifSPSCQueue&) = delete;
    NotifSPSCQueue& operator=(const NotifSPSCQueue&) = delete;
    
    // Disable move
    NotifSPSCQueue(NotifSPSCQueue&&) = delete;
    NotifSPSCQueue& operator=(NotifSPSCQueue&&) = delete;
    
    // Push item by Producer to the end of the queue
    bool tryPush(const T& item);
    
    // Push for move version
    bool tryPush(T&& item);
    
    // Remove/consume item by consumer thread from the front of the queue
    bool tryPop(T &out);

    bool empty() const;
    
    void notifyShutdown();

};

template <typename T>
NotifSPSCQueue<T>::NotifSPSCQueue (std::size_t Capacity): queue_(Capacity),
															has_work_(0)
{

}

template <typename T>
bool NotifSPSCQueue<T>::tryPush(const T& item)
{
	if(queue_.tryPush(item))
	{
		has_work_.release();
		return true;
    }      
    return false;
}

template <typename T>
bool NotifSPSCQueue<T>::tryPush(T&& item)
{
	if(queue_.tryPush(std::move(item)))
	{
		has_work_.release();
		return true;
    }      
    return false;
}

template <typename T>
bool NotifSPSCQueue<T>::tryPop(T &out)
{    
	has_work_.acquire();
	if (queue_.tryPop(out))
	{
		return true;
	}
	return false;
}

template <typename T>
void NotifSPSCQueue<T>::notifyShutdown()
{    
	has_work_.release();
}

template <typename T>
bool NotifSPSCQueue<T>::empty() const 
{
    return queue_.empty();
}

