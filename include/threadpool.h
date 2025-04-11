#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
/*
 * Abstact class Task
 * Users can inherit this class to implement their own task by overriding the run() method
 * */
class Task
{
public:
    virtual void run() = 0;
};

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
    using ThreadFunc = std::function<void()>; // Thread function type
    Thread(ThreadFunc func);
    ~Thread();

    void start();

private:
    ThreadFunc threadFunc_; // Thread function
};

// The Class ThreadPool
class ThreadPool
{
public:
    ThreadPool();
    ~ThreadPool();

    void setMode(PoolMode mode);                 // Set the pool mode
    void setTaskQueMaxSize(size_t size);         // Set the task queue size
    void submitTask(std::shared_ptr<Task> task); // Submit the task to the thread pool
    void start(int initThreadSize = 4);          // Start the thread pool

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

private:
    void threadFunc(); // Thread function
private:
    std::vector<Thread *> threads_; // Threads
    size_t threadSize_;             // Thread Size

    // Users may input temporary task which we need to consider the lifetime of the task
    // So we use intelligent pointer to manage the task
    std::queue<std::shared_ptr<Task>> taskQue_; // Task Queue
    std::atomic_int taskSize_;                  // Task Size
    size_t maxTaskQueSize_;                     // Max Task Queue Size

    std::mutex taskQueMtx_;            // Task Queue Mutex to protect the task queue
    std::condition_variable notEmpty_; // Condition Variable to notify the thread that the task queue is not empty
    std::condition_variable notFull_;  // Condition Variable to notify the thread that the task queue is not full

    PoolMode poolMode_; // Pool Mode
};

#endif