#include "../include/threadpool.h"
#include <thread>
#include <iostream>
const int TASK_MAX_THRESHOLD = 1024;
const int THREAD_MAX_SIZE = 10;
const int THREAD_IDLE_MAX_TIME = 5;
Thread::Thread(ThreadFunc func) : threadFunc_(func), threadId_(generateId_++)
{
}

Thread::~Thread()
{
}
int Thread::generateId_ = 0;
void Thread::start()
{
    // Create a thread to run the threadFunc_
    std::thread t(threadFunc_, threadId_);
    t.detach(); // Detach the thread from the main thread
}

int Thread::getThreadId() const
{
    return threadId_;
}

ThreadPool::ThreadPool() : initThreadSize_(0),
                           maxThreadSize_(THREAD_MAX_SIZE),
                           curThreadSize_(0),
                           idleThreadSize_(0),
                           taskSize_(0),
                           maxTaskQueSize_(TASK_MAX_THRESHOLD),
                           poolMode_(PoolMode::MODE_FIXED),
                           isPoolRunning_(false)
{
}

ThreadPool::~ThreadPool()
{
    isPoolRunning_ = false;
    notEmpty_.notify_all();

    // Two kinds of threads: running and blocked
    std::unique_lock<std::mutex> lock(taskQueMtx_);
    exitCv_.wait(lock, [&]() -> bool
                 { return threads_.size() == 0; });
}

void ThreadPool::setMode(PoolMode mode)
{
    if (checkRunningState())
        return;
    poolMode_ = mode;
}

void ThreadPool::setThreadMaxSize(size_t size)
{
    if (checkRunningState())
        return;
    if (poolMode_ == PoolMode::MODE_CACHED)
        maxThreadSize_ = size;
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
        // 使用移动语义返回Result对象
        return Result(task, false);
    }

    // 3. push the task into the task queue and notify the thread to take the task
    taskQue_.emplace(task);
    taskSize_.fetch_add(1);
    notEmpty_.notify_all();

    // cached Mode
    if (poolMode_ == PoolMode::MODE_CACHED && taskSize_ > idleThreadSize_ && curThreadSize_ < maxThreadSize_)
    {
        std::cout << "Create new thread..." << std::endl;
        auto ptr = std::make_unique<Thread>([this](int threadId)
                                            { this->threadFunc(threadId); });
        int threadId = ptr->getThreadId();
        threads_.emplace(threadId, std::move(ptr));
        threads_[threadId]->start();
        // update relative variables
        curThreadSize_.fetch_add(1);
        idleThreadSize_.fetch_add(1);
    }
    
    // 使用移动语义返回Result对象
    return Result(task, true);
}

// Here using fixed numbers of threads to test. But we can get the number of threads from the OS.
void ThreadPool::start(int initThreadSize)
{
    isPoolRunning_ = true;
    initThreadSize_ = initThreadSize;
    curThreadSize_ = initThreadSize;
    // Create threads
    for (int i = 0; i < initThreadSize_; ++i)
    {
        // Bind the ThreadPool::threadFunc member function to the current ThreadPool object instance (that is, the this pointer).
        // threads_.emplace_back(new Thread(std::bind(&ThreadPool::threadFunc, this)));

        // alternatively, we can use lambda function to bind the threadFunc_
        auto ptr = std::make_unique<Thread>([this](int threadId)
                                            { this->threadFunc(threadId); });
        threads_.emplace(ptr->getThreadId(), std::move(ptr));
        idleThreadSize_.fetch_add(1);
    }

    // Start threads
    for (int i = 0; i < initThreadSize_; ++i)
    {
        threads_[i]->start();
    }
}

// Thread pool get the task and run it
void ThreadPool::threadFunc(int threadId)
{
    // std::cout << "Begin to run the thread function" << std::endl;
    // std::cout << "Thread ID: " << std::this_thread::get_id() << std::endl;
    // std::cout << "The end of the thread function" << std::endl;
    while (isPoolRunning_)
    {
        auto lastTime = std::chrono::high_resolution_clock().now();
        std::shared_ptr<Task> task = nullptr;
        { // 1. get the lock
            std::unique_lock<std::mutex> lock(taskQueMtx_);

            std::cout << "Thread " << std::this_thread::get_id() << " is waiting for the task..." << std::endl;
            // under cached mode, maybe there are many threads. If the idle time > 60s, redundant threads should be over, the part that exceed the initThreadSize.
            while (taskQue_.size() == 0)
            {
                if (poolMode_ == PoolMode::MODE_CACHED)
                {
                    // if conditional variable return timeout
                    if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1)))
                    {
                        auto now = std::chrono::high_resolution_clock().now();
                        auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
                        if (dur.count() >= THREAD_IDLE_MAX_TIME &&
                            curThreadSize_ > initThreadSize_)
                        {
                            // recycle current threads
                            threads_.erase(threadId);
                            curThreadSize_.fetch_sub(1);
                            idleThreadSize_.fetch_sub(1);
                            std::cout << "Thread " << threadId << " is idle for too long, recycle..." << std::endl;
                            return;
                        }
                    }
                }
                else
                {
                    // 2. wait until the task queue is not empty
                    notEmpty_.wait(lock);
                }
                // 3. check if the thread pool is running, if not, exit the thread
                if (!isPoolRunning_)
                {
                    threads_.erase(threadId);
                    std::cout << "Thread " << threadId << " is exiting..." << std::endl;
                    exitCv_.notify_all();
                    return;
                }
            }
            idleThreadSize_.fetch_sub(1);

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

        idleThreadSize_.fetch_add(1);
    }

    // 6. exit the thread
    threads_.erase(threadId);
    std::cout << "Thread " << threadId << " is exiting..." << std::endl;
    exitCv_.notify_all();
}

bool ThreadPool::checkRunningState() const
{
    return isPoolRunning_;
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
