#pragma once

#include <queue>
#include <mutex>
#include <utility>

static constexpr std::size_t DEFAULT_Q_CAPACITY = 10;

template<typename T>
class ConcurrentQueue {

private:
    std::size_t capacity_;
    std::queue<T> queue_;
    mutable std::mutex mtx_;
    
public:
	explicit ConcurrentQueue(std::size_t capacity = DEFAULT_Q_CAPACITY);
	
	// disable copying
	ConcurrentQueue(const ConcurrentQueue&) = delete;
	ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;
	
	// disable moving
	ConcurrentQueue(ConcurrentQueue &&) = delete;
	ConcurrentQueue& operator=(ConcurrentQueue&&) = delete;
	
	// for when task is an lvalue
	bool try_push(const T& item);
	
	// if task is an rvalue
	bool try_push(T&& item);
	
	bool try_pop(T& item);
	
	bool empty() const ;
	
	std::size_t size() const ;

};

template<typename T>
ConcurrentQueue<T>::ConcurrentQueue(std::size_t capacity): capacity_(capacity) {

	// TODO: if queue requested of invalid size , fail consreuctor
}


template<typename T>
bool ConcurrentQueue<T>::try_push(const T& item)
{
	std::lock_guard<std::mutex> lock(mtx_);
	
	if (queue_.size() >= capacity_)
		return false;
		
	queue_.push(item);
	
	return true;
}

template<typename T>
bool ConcurrentQueue<T>::try_push(T&& item)
{
	std::lock_guard<std::mutex> lock(mtx_);
	
	if (queue_.size() >= capacity_)
		return false;
		
	queue_.push(std::move(item));
	
	return true;
}


template<typename T>
bool ConcurrentQueue<T>::try_pop(T& item)
{
	std::lock_guard<std::mutex> lock(mtx_);
	
	if (queue_.empty())
		return false;
		
	item = std::move(queue_.front());
	
	queue_.pop();
	
	return true;

}

template<typename T>
bool ConcurrentQueue<T>::empty() const
{
	std::lock_guard<std::mutex> lock(mtx_);
	return queue_.empty();
}

template<typename T>
std::size_t ConcurrentQueue<T>::size() const
{
	std::lock_guard<std::mutex> lock(mtx_);
	return queue_.size();
}










