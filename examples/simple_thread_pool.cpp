#include <iostream>
#include <mutex>
#include <thread>
#include "thread_pool_with_return.h"

using namespace std;

template <typename Task, typename Func, typename... Args>
Task constructTask(Func&& func, Args&&... args)
{
    Task task = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    return task;
}

void  worker(string s) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    unique_lock<mutex>  lock(cout_mtx);
    cout << "Worker thread: " << std::this_thread::get_id() <<"  " <<s << endl;
    lock.unlock();
}

std::string worker_return_string(const std::string &s) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    unique_lock<mutex>  lock(cout_mtx);
    cout << "Worker thread: " << std::this_thread::get_id() <<"  " <<s << endl;
    lock.unlock();
    return s+"_returned" ;
}

void test_fire_and_forget_tasks()
{
    int n=10;
    ThreadPool_Q myThreadPool(n, 4);

    vector<string> fruits {"Apple", "Banana", "Pear", "Mango", "Guava", "Kiwi", "Orange", "Melon", "Papaya","Pineapple"};
    for (int i=0;i<n;i++)
    {   
        std::function<void()> task = constructTask<std::function<void()>>(worker,fruits[i]);
        myThreadPool.tryPush(std::move(task));
    }
}

void test_task_with_string_returns()
{
    int n=10;
    ThreadPool_Q myThreadPool(10, 4);

    std::vector<std::future<std::string>> results;
    vector<string> fruits {"Apple", "Banana", "Pear", "Mango", "Guava", "Kiwi", "Orange", "Melon", "Papaya","Pineapple"};
    for (int i=0;i<10;i++)
    {   
        results.push_back(myThreadPool.pushTask(worker_return_string,fruits[i]));
    }
    for(int i=0;i<n;i++)
    {
        cout << results[i].get() <<endl;
    }
}

int main()
{
    test_fire_and_forget_tasks();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cout << "----------------------------------------------------" << std::endl;
    test_task_with_string_returns();

    // No need to wait for a while to let the tasks complete, as threadPool destructor will take care of waiting till all threads are joined

    return 0;
}