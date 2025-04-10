#include "threadpool.h"

const int TASK_MAX_THRESHOLD = 1024;
Thread::Thread()
{
}

Thread::~Thread()
{
}

void Thread::start()
{
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

void ThreadPool::submitTask(std::shared_ptr<Task> task)
{
}

// Here using fixed numbers of threads to test. But we can get the number of threads from the OS.
void ThreadPool::start(int initThreadSize)
{
    threadSize_ = initThreadSize;
    // Create threads
    for(int i = 0; i < threadSize_; ++i){
        threads_.emplace_back(new Thread);
    }

    // Start threads
    for(int i = 0; i < threadSize_; ++i){
        threads_[i]->start();
    }
}
