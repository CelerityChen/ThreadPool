#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <unordered_map>

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

    // 添加移动构造函数
    Semaphore(Semaphore &&other) noexcept
        : resLimit_(other.resLimit_)
    {
        other.resLimit_ = 0;
    }

    // 添加移动赋值运算符
    Semaphore &operator=(Semaphore &&other) noexcept
    {
        if (this != &other)
        {
            resLimit_ = other.resLimit_;
            other.resLimit_ = 0;
        }
        return *this;
    }

    // 显式删除拷贝构造函数和拷贝赋值运算符
    Semaphore(const Semaphore &) = delete;
    Semaphore &operator=(const Semaphore &) = delete;

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

    // 自定义移动构造函数
    Result(Result &&other) noexcept
        : any_(std::move(other.any_)),
          sem_(std::move(other.sem_)),
          task_(std::move(other.task_)),
          isValid_(other.isValid_.load())
    {
        other.isValid_ = false;
    }

    // 自定义移动赋值运算符
    Result &operator=(Result &&other) noexcept
    {
        if (this != &other)
        {
            any_ = std::move(other.any_);
            sem_ = std::move(other.sem_);
            task_ = std::move(other.task_);
            isValid_ = other.isValid_.load();
            other.isValid_ = false;
        }
        return *this;
    }

    // 显式删除拷贝构造函数和拷贝赋值运算符
    Result(const Result &) = delete;
    Result &operator=(const Result &) = delete;

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
    using ThreadFunc = std::function<void(int)>; // Thread function type
    Thread(ThreadFunc func);
    ~Thread();

    void start();
    int getThreadId() const;

private:
    ThreadFunc threadFunc_; // Thread function
    static int generateId_; // Used to generate the thread id
    int threadId_;          // Used to index the current thread
};

// The Class ThreadPool
class ThreadPool
{
public:
    ThreadPool();
    ~ThreadPool();

    void setMode(PoolMode mode);                                          // Set the pool mode
    void setThreadMaxSize(size_t size);                                   // Set the max thread size
    void setTaskQueMaxSize(size_t size);                                  // Set the task queue size
    Result submitTask(std::shared_ptr<Task> task);                        // Submit the task to the thread pool
    void start(int initThreadSize = std::thread::hardware_concurrency()); // Start the thread pool

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

private:
    void threadFunc(int threadId);  // Thread function
    bool checkRunningState() const; // Check the state of the poo
private:
    std::unordered_map<int, std::unique_ptr<Thread>> threads_; // Threads
    size_t initThreadSize_;                                    // Thread Size
    size_t maxThreadSize_;                                     // Max Thread Size
    std::atomic_int curThreadSize_;                            // current number of threads
    std::atomic_int idleThreadSize_;                           // Number of idle threads

    // Users may input temporary task which we need to consider the lifetime of the task
    // So we use intelligent pointer to manage the task
    std::queue<std::shared_ptr<Task>> taskQue_; // Task Queue
    std::atomic_int taskSize_;                  // Task Size
    size_t maxTaskQueSize_;                     // Max Task Queue Size

    std::mutex taskQueMtx_;            // Task Queue Mutex to protect the task queue
    std::condition_variable notEmpty_; // Condition Variable to notify the thread that the task queue is not empty
    std::condition_variable notFull_;  // Condition Variable to notify the thread that the task queue is not full
    std::condition_variable exitCv_;   // Condition Variable to notify the thread that the thread pool is exiting

    PoolMode poolMode_;              // Pool Mode
    std::atomic_bool isPoolRunning_; // state of the pool running or not
};

#endif