#pragma once

#include <queue>
#include <mutex>
#include <utility>
#include <vector>
#include <cstddef>

// for diag
#include <atomic>
#include <chrono>

static constexpr std::size_t DEFAULT_Q_CAPACITY = 10;

template<typename T>
class ConcurrentQueue {

private:
    std::size_t capacity_;
    std::queue<T> queue_;
    mutable std::mutex mtx_;
    
public:

    //for debug purpose only
   	std::atomic<unsigned int> push_stats;
 	std::atomic<unsigned int> pop_stats;
    std::chrono::nanoseconds pop_contention_time{0};
    std::chrono::nanoseconds push_contention_time{0};
    std::atomic<unsigned int> batches_count;
    
	explicit ConcurrentQueue(std::size_t capacity = DEFAULT_Q_CAPACITY);
	
	// disable copying
	ConcurrentQueue(const ConcurrentQueue&) = delete;
	ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;
	
	// disable moving
	ConcurrentQueue(ConcurrentQueue &&) = delete;
	ConcurrentQueue& operator=(ConcurrentQueue&&) = delete;
	
	// for when task is an lvalue
	bool tryPush(const T& item);
	
	// if task is an rvalue
	bool tryPush(T&& item);
	
	bool tryPop(T& item);
	
	// pop and apppend upto batch_size number of elements to items and return number appended, already existing elements in items are preserved
	std::size_t  tryPopBatch(std::vector<T>& items, std::size_t  batch_size);
	
	bool empty() const ;
	
	std::size_t size() const ;

};

template<typename T>
ConcurrentQueue<T>::ConcurrentQueue(std::size_t capacity): capacity_(capacity),
														// for diag
														push_stats(0), pop_stats(0),
														batches_count(0)
{	
	/*
		Let queue capacity =0 be valid for now , we may need it for correctness and other diagnostic experiments
	*/
}


template<typename T>
bool ConcurrentQueue<T>::tryPush(const T& item)
{

	//************* diagnostics start *************************
	if (!mtx_.try_lock())
	{
		push_stats.fetch_add(1, std::memory_order_relaxed);
		auto start = std::chrono::steady_clock::now();
		mtx_.lock();
		auto end = std::chrono::steady_clock::now();
		
		push_contention_time += std::chrono::duration_cast<std::chrono::nanoseconds>(end-start);
	}
	std::lock_guard<std::mutex> lock(mtx_,std::adopt_lock);
	//-------------- diagnostics end *-----------------------
	
	//std::lock_guard<std::mutex> lock(mtx_);
	
	if (queue_.size() >= capacity_)
		return false;
	
	queue_.push(item);
	return true;
}

template<typename T>
bool ConcurrentQueue<T>::tryPush(T&& item)
{
	//************* diagnostics start *************************
	if (!mtx_.try_lock())
	{
		push_stats.fetch_add(1, std::memory_order_relaxed);
		auto start = std::chrono::steady_clock::now();
		mtx_.lock();
		auto end = std::chrono::steady_clock::now();
		
		push_contention_time += std::chrono::duration_cast<std::chrono::nanoseconds>(end-start);
	}
	std::lock_guard<std::mutex> lock(mtx_,std::adopt_lock);	
	//-------------- diagnostics end *-----------------------

	//std::lock_guard<std::mutex> lock(mtx_);
	
	if (queue_.size() >= capacity_)
		return false;
		
	queue_.push(std::move(item));
	return true;
}


template<typename T>
bool ConcurrentQueue<T>::tryPop(T& item)
{
	//************* diagnostics start *************************
	if (!mtx_.try_lock())
	{
		pop_stats.fetch_add(1, std::memory_order_relaxed);
		auto start = std::chrono::steady_clock::now();
		mtx_.lock();
		auto end = std::chrono::steady_clock::now();
		
		pop_contention_time += std::chrono::duration_cast<std::chrono::nanoseconds>(end-start);
	}
	std::lock_guard<std::mutex> lock(mtx_,std::adopt_lock);
	
	//-------------- diagnostics end *-----------------------
	
	//std::lock_guard<std::mutex> lock(mtx_);
	
	if (!queue_.empty())
	{
		item = std::move(queue_.front());
		queue_.pop();
		return true;
	}
	return false;

}

template<typename T>
std::size_t  ConcurrentQueue<T>::tryPopBatch(std::vector<T>& items,std::size_t batch_size)
{
	std::size_t work_popped_count = 0;

	if (!batch_size)
		return 0 ;
	
	//************* diagnostics start *************************	
	batches_count.fetch_add(1, std::memory_order_relaxed);
	
	if (!mtx_.try_lock())
	{
		pop_stats.fetch_add(1, std::memory_order_relaxed);
		auto start = std::chrono::steady_clock::now();
		mtx_.lock();
		auto end = std::chrono::steady_clock::now();
		
		pop_contention_time += std::chrono::duration_cast<std::chrono::nanoseconds>(end-start);
	}
	std::lock_guard<std::mutex> lock(mtx_,std::adopt_lock);
	//-------------- diagnostics end *-----------------------
	
	//std::lock_guard<std::mutex> lock(mtx_);
	
	while(!queue_.empty() && (work_popped_count < batch_size) )
	{
		items.push_back(std::move(queue_.front()));
		queue_.pop();
		work_popped_count++;
	}
	
	return work_popped_count;

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










