#include <iostream>
#include <mutex>
#include <thread>
#include "simple_thread_pool.h"

using namespace std;

void  worker(string s) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    unique_lock<mutex>  lock(cout_mtx);
    cout << "Worker thread: " << std::this_thread::get_id() <<"  " <<s << endl;
    lock.unlock();
   
    //return 0;
}

int main()
{

    ThreadPool_Q myThreadPool(10, 4);
    int n=10;
    //std::function<void()> myTask = worker;
    std::vector<std::future<void>> results;
    vector<string> fruits {"Apple", "Banana", "Pear", "Mango", "Guava", "Kiwi", "Orange", "Melon", "Papaya","Pineapple"};
    for (int i=0;i<10;i++)
    {   
        //myThreadPool.pushTask([](string s){cout << s << endl;},fruits[i]);
        results.push_back(myThreadPool.pushTask(worker,fruits[i]));
        //cout << fut.get()<<endl;
    }
    for(int i=0;i<10;i++)
        results[i].get();
        //cout << results[i].get() <<endl;
    

    // No need to wait for a while to let the tasks complete, as threadPool destructor will take care of waiting till all threads are joined

    return 0;
}