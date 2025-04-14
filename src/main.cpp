#include "threadpool.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <memory>

/*
 * Example:
 * ThreadPool pool;
 * pool.start(4);
 *
 * class MyTask : public Task{
 * public:
 *      void run(){
 *          // Task body, like downloading, processing, etc.
 *          std::cout << "Task is running" << std::endl;
 *      }
 * };
 *
 * pool.submitTask(std::make_shared<MyTask>());
 *
 *
 * */

// 示例1：返回int类型的任务
class AddTask : public Task
{
public:
    AddTask(int a, int b) : a_(a), b_(b) {}
    Any run() override
    {
        std::cout << "AddTask running in thread: " << std::this_thread::get_id() << std::endl;
        unsigned long long sum = 0;
        for (int i = a_; i <= b_; ++i)
        {
            sum += i;
        }
        return sum;
    }

private:
    int a_;
    int b_;
};

// 示例2：返回string类型的任务
class ConcatTask : public Task
{
public:
    ConcatTask(const std::string &a, const std::string &b)
        : a_(a), b_(b) {}
    Any run() override
    {
        std::cout << "ConcatTask running in thread: " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 模拟耗时操作
        return a_ + b_;
    }

private:
    std::string a_;
    std::string b_;
};

int main()
{
    // 创建线程池
    ThreadPool pool;
    pool.start(4);

    Result res1 = pool.submitTask(std::make_shared<AddTask>(1, 100000000));
    Result res2 = pool.submitTask(std::make_shared<AddTask>(100000001, 200000000));
    Result res3 = pool.submitTask(std::make_shared<AddTask>(200000001, 300000000));

    unsigned long long sum = res1.get().cast_<unsigned long long>() + res2.get().cast_<unsigned long long>() + res3.get().cast_<unsigned long long>();
    std::cout << "Sum: " << sum << std::endl;

    return 0;
}
