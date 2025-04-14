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
Result ThreadPool::submitTask(std::shared_ptr<Task> task)
{
    // 1. lock the task queue
    std::unique_lock<std::mutex> lock(taskQueMtx_);

    // 2. 等待任务队列不满，最多等待1秒
    if (!notFull_.wait_for(lock, std::chrono::seconds(1),
                           [&]() -> bool
                           { return taskQue_.size() < maxTaskQueSize_; }))
    {
        std::cerr << "Task queue is full, submit task failed" << std::endl;
        // 等待超时，返回false表示提交失败
        return Result(task, false);
    }

    // 3. push the task into the task queue and notify the thread to take the task
    taskQue_.emplace(task);
    taskSize_.fetch_add(1);
    notEmpty_.notify_all();

    return Result(task, true);
}

// Here using fixed numbers of threads to test. But we can get the number of threads from the OS.
void ThreadPool::start(int initThreadSize)
{
    threadSize_ = initThreadSize;
    // Create threads
    for (int i = 0; i < threadSize_; ++i)
    {
        // Bind the ThreadPool::threadFunc member function to the current ThreadPool object instance (that is, the this pointer).
        // threads_.emplace_back(new Thread(std::bind(&ThreadPool::threadFunc, this)));

        // alternatively, we can use lambda function to bind the threadFunc_
        auto ptr = std::make_unique<Thread>([this]()
                                            { this->threadFunc(); });
        threads_.emplace_back(std::move(ptr));
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
    // std::cout << "Begin to run the thread function" << std::endl;
    // std::cout << "Thread ID: " << std::this_thread::get_id() << std::endl;
    // std::cout << "The end of the thread function" << std::endl;
    for (;;)
    {
        std::shared_ptr<Task> task = nullptr;
        { // 1. get the lock
            std::unique_lock<std::mutex> lock(taskQueMtx_);

            std::cout << "Thread " << std::this_thread::get_id() << " is waiting for the task..." << std::endl;

            // 2. wait until the task queue is not empty
            notEmpty_.wait(lock, [&]() -> bool
                           { return !taskQue_.empty(); });

            std::cout << "Thread " << std::this_thread::get_id() << " get the task successfully." << std::endl;
            // 3. get the task from the task queue
            task = taskQue_.front();
            taskQue_.pop();
            taskSize_.fetch_sub(1);

            // if there are still tasks in the task queue, notify the thread to take the task
            if (taskQue_.size() > 0)
            {
                notEmpty_.notify_all();
            }

            // notify the task queue is not full
            notFull_.notify_all();
        } // 4. End of the critical section, unlock the task queue

        // 5. run the task
        if (task != nullptr)
            task->exec();
    }
}

Result::Result(std::shared_ptr<Task> task, bool isValid) : isValid_(isValid), task_(task)
{
    // 一开始没将Result和Task关联起来，导致没有得到打印结果。
    task_->setResult(this);
}

Result::~Result()
{
}

void Result::setVal(Any any)
{
    this->any_ = std::move(any);
    sem_.post();
}

Any Result::get()
{
    if (!isValid_)
    {
        return Any();
    }

    sem_.wait();
    return std::move(any_);
}

Task::Task() : result_(nullptr)
{
}

void Task::exec()
{
    if (result_ != nullptr)
    {
        result_->setVal(run());
    }
}

void Task::setResult(Result *res)
{
    result_ = res;
}
