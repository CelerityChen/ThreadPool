#include "threadpool.h"
#include <thread>
#include <iostream>
const int TASK_MAX_THRESHOLD = 1024;
Thread::Thread(ThreadFunc func) : threadFunc_(func)
{
}

Thread::~Thread()
{
}

void Thread::start()
{
    // Create a thread to run the threadFunc_
    std::thread t(threadFunc_);
    t.detach(); // Detach the thread from the main thread
}

ThreadPool::ThreadPool() : threadSize_(0),
                           taskSize_(0),
                           maxTaskQueSize_(TASK_MAX_THRESHOLD),
                           poolMode_(PoolMode::MODE_FIXED)
{
}

ThreadPool::~ThreadPool()
{
}

void ThreadPool::setMode(PoolMode mode)
{
    poolMode_ = mode;
}

void ThreadPool::setTaskQueMaxSize(size_t size)
{
    maxTaskQueSize_ = size;
}

// Users input the task to the thread pool
void ThreadPool::submitTask(std::shared_ptr<Task> task)
{
}

// Here using fixed numbers of threads to test. But we can get the number of threads from the OS.
void ThreadPool::start(int initThreadSize)
{
    threadSize_ = initThreadSize;
    // Create threads
    for (int i = 0; i < threadSize_; ++i)
    {
        // Bind the ThreadPool::threadFunc member function to the current ThreadPool object instance (that is, the this pointer).
        threads_.emplace_back(new Thread(std::bind(&ThreadPool::threadFunc, this)));
    }

    // Start threads
    for (int i = 0; i < threadSize_; ++i)
    {
        threads_[i]->start();
    }
}

// Thread pool get the task and run it
void ThreadPool::threadFunc()
{
    std::cout << "Begin to run the thread function" << std::endl;
    std::cout << "Thread ID: " << std::this_thread::get_id() << std::endl;
    std::cout << "The end of the thread function" << std::endl;
}