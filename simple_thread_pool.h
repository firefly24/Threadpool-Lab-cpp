#ifndef SIMPLE_THREADPOOL_H
#define SIMPLE_THREADPOOL_H
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <vector>
#include <future>

using namespace std;

static std::mutex cout_mtx;
typedef std::function<void()> T;

class ThreadPool_Q
{
private:
    std::size_t capacity;
    std::size_t maxWorkers;
    std::queue<T> taskList;
    std::mutex mtx;
    std::condition_variable cond_;
    std::vector<std::thread> worker_threads;
    std::atomic<unsigned int> completed_tasks;
    std::atomic<bool> stop_pool;

    // Disable copying
    ThreadPool_Q(const ThreadPool_Q &) = delete;
    ThreadPool_Q &operator=(const ThreadPool_Q &) = delete;

    // Disable moving
    ThreadPool_Q(ThreadPool_Q &&) = delete;
    ThreadPool_Q &operator=(ThreadPool_Q &&) = delete;

    // Worker thread function to pop tasks from the queue and execute them
    void startWorkerThread() noexcept
    {
        T task;
        // keep polling for new tasks on this thread
        while (1)
        {
            unique_lock<mutex> mLock(mtx);
            // Wait until there is a task in the queue
            cond_.wait(mLock, [this]()
                       { return !taskList.empty() || stop_pool.load(std::memory_order_acquire); });

            // return only if pool is stopped and all tasks are completed
            if (stop_pool.load(std::memory_order_acquire) && taskList.empty())
                return;

            // Pop a task from the queue to attach to current worker thread
            task = std::move(taskList.front());
            taskList.pop();
            mLock.unlock();
            // Execute the task
            try
            {
                task();
            }
            catch (...)
            {
                std::cerr << "Task thown exception" << std::endl;
            }
            // Notify that a task has been completed
            completed_tasks++;
        }
    }

public:
    // Constructor launch worker threads
    explicit ThreadPool_Q(std::size_t task_capacity, std::size_t max_workers) : capacity(task_capacity), maxWorkers(max_workers)
    {
        completed_tasks = 0;
        stop_pool.store(false, std::memory_order_release);
        // Initialize worker threads
        for (size_t i = 0; i < maxWorkers; i++)
            worker_threads.emplace_back([this]
                                        { startWorkerThread(); }); // lamba fn to pop a task from queue to execute
    }

    int completedTaskCount()
    {
        return completed_tasks.load();
    }

    // Push a task into the queue, and return a future object which other programs can use to fetch results of the task
    // need to declare the return type as std::future explicitly, as auto is deducing it as "void"
    template <typename Func, typename... Args>
    auto pushTask(Func &&func, Args &&...args) -> std::future<decltype(func(args...))>
    {
        // We need to encapsulate the function such that calling func() will be equivalent to calling func(args...)
        // To do this, we can bind the args.. to func object by: auto task =  std::bind(func,args...);
        // But, we need to preserve the lvalue/rvalue typeof arguments, so we need to use perfect forwarding
        // auto task = std::bind(std::forward<Func>(func),std::forward<Args>(args)...);

        // Since we want the threadpool to run any function with any return type, the decltype() will deduce the return type
        using return_type = decltype(func(args...));
        // to add support for tasks to return a value, use packaged_task to get the std::future object
        // std::packaged_task<return_type()> pkg_task(std::bind(std::forward<Func>(func),std::forward<Args>(args)...));

        // since the packaged_task is non-copyable, we cannot pass it as parameter, so we use std::move
        //  We want the packaged_task object to persist in the queue even this function ends, so we encapsulate it with smart_ptr
        // auto encapsulated_pkTask = std::make_shared<std::packaged_task<return_type()>>(std::move(pkg_task));

        // OR We can use this to construct packaged task in-place
        auto task = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
        auto survivorPtr_pkTask = std::make_shared<std::packaged_task<return_type()>>(std::move(task));

        std::future<return_type> result = survivorPtr_pkTask->get_future();

        std::unique_lock<std::mutex> mLock(mtx);
        // Wait until there is space in the queue
        if (!cond_.wait_for(mLock, std::chrono::seconds(20), [this]()
                            { return (taskList.size() < capacity) || stop_pool.load(std::memory_order_acquire); }))
            throw std::runtime_error("Timeout! Queue is full.");

        // If pool is stopped, do no not push any tasks to the queue
        // TODO: implement stop condition
        if (stop_pool.load(std::memory_order_acquire))
            throw std::runtime_error("Cannot enqueue new tasks as thread pool is stopped");
        taskList.emplace([survivorPtr_pkTask]()
                         { (*survivorPtr_pkTask)(); });
        mLock.unlock();
        cond_.notify_one();
        return result;
    }

    // Fire and forget tasks
    bool tryPush(T &&task)
    {
        std::unique_lock<std::mutex> mLock(mtx);
        if (stop_pool.load(std::memory_order_acquire) || (taskList.size() >= capacity))
            return false;
        taskList.push(std::forward<T>(task));
        cond_.notify_one();
        return true;
    }

    void stopPool()
    {
        std::unique_lock<std::mutex> mLock(mtx);
        stop_pool.store(true, std::memory_order_release);
        mLock.unlock();
        cond_.notify_all(); // Notify all worker threads to wake up and check the stop condition

        // Join all worker threads
    }

    ~ThreadPool_Q()
    {
        // Stop the pool and notify all worker threads
        std::cout << "ThreadPool stopped" << std::endl;

        if (stop_pool.exchange(false))
            stopPool();
        
        for (auto &worker : worker_threads)
            worker.join();
    }
};

#endif /* SIMPLE_THREADPOOL_H */