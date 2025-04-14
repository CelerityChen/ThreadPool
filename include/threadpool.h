#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>

// This code implements a custom Any class similar to std::any in C++17. Its core idea is to store values of any Type through Type Erasure technique.
class Any
{
public:
    Any() = default;
    ~Any() = default;
    Any(const Any &) = delete;
    Any &operator=(const Any &) = delete;
    Any(Any &&) = default;
    Any &operator=(Any &&) = default;

    template <typename T>
    Any(T data) : base_(std::make_unique<Derived<T>>(data)) {}

    template <typename T>
    T cast_()
    {
        Derived<T> *ptr = dynamic_cast<Derived<T> *>(base_.get());
        if (ptr == nullptr)
        {
            throw std::bad_cast();
        }
        return ptr->data_;
    }

private:
    // base class
    class Base
    {
    public:
        virtual ~Base() = default;
    };

    // derived class
    template <typename T>
    class Derived : public Base
    {
    public:
        Derived(T data) : data_(data) {}
        T data_;
    };

private:
    std::unique_ptr<Base> base_;
};

// Implement the Semaphore class
class Semaphore
{
public:
    Semaphore(int limit = 0) : resLimit_(limit) {};
    ~Semaphore() = default;
    // wait for the resource
    void wait()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [&]() -> bool
                 { return resLimit_ > 0; });
        --resLimit_;
    }
    // post the resource
    void post()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        ++resLimit_;
        cv_.notify_all();
    }

private:
    int resLimit_;
    std::mutex mtx_;
    std::condition_variable cv_;
};

// Forward declaration of the Task class
class Task;

// Implements the class that receives the value returned by the thread
class Result
{
public:
    Result(std::shared_ptr<Task> task, bool isValid = true);
    ~Result();
    void setVal(Any any);
    Any get();

private:
    Any any_;
    Semaphore sem_;
    std::shared_ptr<Task> task_;
    std::atomic_bool isValid_;
};
/*
 * Abstact class Task
 * Users can inherit this class to implement their own task by overriding the run() method
 * */
class Task
{
public:
    Task();
    ~Task() = default;
    void exec();
    void setResult(Result *res);
    virtual Any run() = 0;

private:
    Result *result_;
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

    void setMode(PoolMode mode);                   // Set the pool mode
    void setTaskQueMaxSize(size_t size);           // Set the task queue size
    Result submitTask(std::shared_ptr<Task> task); // Submit the task to the thread pool
    void start(int initThreadSize = 4);            // Start the thread pool

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

private:
    void threadFunc(); // Thread function
private:
    std::vector<std::unique_ptr<Thread>> threads_; // Threads
    size_t threadSize_;                            // Thread Size

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