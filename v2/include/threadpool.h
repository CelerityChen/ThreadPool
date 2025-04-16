#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <unordered_map>
#include <future>
#include <chrono>

const int TASK_MAX_THRESHOLD = 1024;
const int THREAD_MAX_SIZE = 10;
const int THREAD_IDLE_MAX_TIME = 5;
// Supporting Mode
enum class PoolMode
{
    MODE_FIXED,
    MODE_CACHED,
};

// The Class Thread
class Thread
{
public:
    using ThreadFunc = std::function<void(int)>; // Thread function type
    Thread(ThreadFunc func) : threadFunc_(func), threadId_(generateId_++) {}
    ~Thread() = default;

    void start()
    {
        std::thread t(threadFunc_, threadId_);
        t.detach();
    }

    int getThreadId() const { return threadId_; }

private:
    ThreadFunc threadFunc_; // Thread function
    static int generateId_; // Used to generate the thread id
    int threadId_;          // Used to index the current thread
};

int Thread::generateId_ = 0;

// The Class ThreadPool
class ThreadPool
{
public:
    ThreadPool() : initThreadSize_(0),
                   maxThreadSize_(THREAD_MAX_SIZE),
                   curThreadSize_(0),
                   idleThreadSize_(0),
                   taskSize_(0),
                   maxTaskQueSize_(TASK_MAX_THRESHOLD),
                   poolMode_(PoolMode::MODE_FIXED),
                   isPoolRunning_(false) {}
    ~ThreadPool()
    {
        isPoolRunning_ = false;

        std::unique_lock<std::mutex> lock(taskQueMtx_);
        notEmpty_.notify_all();
        exitCv_.wait(lock, [&]() -> bool
                     { return threads_.size() == 0; });
    }

    void setMode(PoolMode mode)
    {
        if (checkRunningState())
            return;
        poolMode_ = mode;
    }

    void setThreadMaxSize(size_t size)
    {
        if (checkRunningState())
            return;
        maxThreadSize_ = size;
    }

    void setTaskQueMaxSize(size_t size)
    {
        if (checkRunningState())
            return;
        maxTaskQueSize_ = size;
    }

    template <typename Func, typename... Args>
    auto submitTask(Func &&func, Args &&...args) -> std::future<decltype(func(args...))>
    {
        if (!checkRunningState())
            throw std::runtime_error("ThreadPool is not running");
        using RType = decltype(func(args...));
        auto task = std::make_shared<std::packaged_task<RType()>>(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
        std::future<RType> res = task->get_future();

        std::unique_lock<std::mutex> lock(taskQueMtx_);
        if (!notFull_.wait_for(lock, std::chrono::seconds(1),
                               [&]() -> bool
                               { return taskSize_ < maxTaskQueSize_; }))
        {
            // throw std::runtime_error("Task queue is full");
            std::cerr << "Task queue is full, submit task failed" << std::endl;
            auto temp = std::make_shared<std::packaged_task<RType()>>(
                []() -> RType
                { return RType(); });
            (*temp)();
            return temp->get_future();
        }

        // using Task = std::function<void()>;
        // taskQue_.emplace([task, args = std::make_tuple(std::forward<Args>(args)...)]()
        //                  { std::apply([&task](auto&&... params) { (*task)(std::forward<decltype(params)>(params)...); }, args); });
        taskQue_.emplace([task]()
                         { (*task)(); });

        ++taskSize_;
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
            ++curThreadSize_;
            ++idleThreadSize_;
        }
        return res;
    }

    void start(int initThreadSize = std::thread::hardware_concurrency())
    {
        isPoolRunning_ = true;
        initThreadSize_ = initThreadSize;
        curThreadSize_ = initThreadSize;

        // Create threads
        for (int i = 0; i < initThreadSize_; ++i)
        {
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

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

private:
    void threadFunc(int threadId)
    {
        auto lastTime = std::chrono::high_resolution_clock().now();
        //
        for (;;)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(taskQueMtx_);

                std::cout << "Thread " << std::this_thread::get_id() << " is waiting for the task..." << std::endl;
                while (taskSize_ == 0 && isPoolRunning_)
                {
                    if (poolMode_ == PoolMode::MODE_CACHED)
                    {
                        if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1)))
                        {
                            auto now = std::chrono::high_resolution_clock().now();
                            auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
                            if (dur.count() >= THREAD_IDLE_MAX_TIME && curThreadSize_ > initThreadSize_)
                            {
                                threads_.erase(threadId);
                                --curThreadSize_;
                                --idleThreadSize_;
                                std::cout << "Thread " << threadId << " is idle for too long, recycle..." << std::endl;
                                return;
                            }
                        }
                    }
                    else
                    {
                        notEmpty_.wait(lock);
                    }
                }
                if (!isPoolRunning_)
                {
                    threads_.erase(threadId);
                    std::cout << "Thread " << threadId << " is exiting..." << std::endl;
                    exitCv_.notify_all();
                    return;
                }
                // Get the task from the task queue
                task = taskQue_.front();
                taskQue_.pop();
                --taskSize_;
                --idleThreadSize_;
                std::cout << "Thread " << threadId << " get the task successfully." << std::endl;
                notFull_.notify_all();

                // if there are still tasks in the task queue, notify the thread to take the task
                if (taskSize_ > 0)
                {
                    notEmpty_.notify_all();
                }
            }
            // Execute the task
            if (task)
                task();
            idleThreadSize_.fetch_add(1);
            lastTime = std::chrono::high_resolution_clock().now();
        }
    } // Thread function

    bool checkRunningState() const
    {
        return isPoolRunning_;
    }; // Check the state of the poo

private:
    std::unordered_map<int, std::unique_ptr<Thread>> threads_; // Threads
    size_t initThreadSize_;                                    // Thread Size
    size_t maxThreadSize_;                                     // Max Thread Size
    std::atomic_int curThreadSize_;                            // current number of threads
    std::atomic_int idleThreadSize_;                           // Number of idle threads

    // Users may input temporary task which we need to consider the lifetime of the task
    // so we use intelligent pointer to manage the task
    using Task = std::function<void()>;
    std::queue<Task> taskQue_; // Task Queue
    std::atomic_int taskSize_; // Task Size
    size_t maxTaskQueSize_;    // Max Task Queue Size

    std::mutex taskQueMtx_;            // Task Queue Mutex to protect the task queue
    std::condition_variable notEmpty_; // Condition Variable to notify the thread that the task queue is not empty
    std::condition_variable notFull_;  // Condition Variable to notify the thread that the task queue is not full
    std::condition_variable exitCv_;   // Condition Variable to notify the thread that the thread pool is exiting

    PoolMode poolMode_;              // Pool Mode
    std::atomic_bool isPoolRunning_; // state of the pool running or not
};

#endif