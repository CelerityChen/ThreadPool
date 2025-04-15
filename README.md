# ThreadPool 线程池项目

## 项目说明
这是一个基于C++11实现的线程池项目，提供了固定大小和动态缓存的两种线程池模式。项目使用现代C++特性，包括智能指针、lambda表达式、条件变量等，实现了一个高效、易用的线程池框架。

## 类结构说明

### 1. Task 类
- 抽象基类，用于定义任务接口
- 纯虚函数 `run()` 需要由用户继承实现
- 使用智能指针 `std::shared_ptr<Task>` 管理任务生命周期

### 2. Thread 类
- 线程封装类
- 主要成员：
  - `ThreadFunc threadFunc_`：线程执行函数
  - `start()`：启动线程的方法
- 使用 `std::thread` 实现线程创建和管理

### 3. ThreadPool 类
- 线程池核心类
- 主要成员：
  - `threads_`：线程集合
  - `taskQue_`：任务队列
  - `taskSize_`：当前任务数量
  - `maxTaskQueSize_`：最大任务队列大小
  - `poolMode_`：线程池模式（固定/缓存）
- 主要方法：
  - `setMode()`：设置线程池模式
  - `setTaskQueMaxSize()`：设置任务队列大小
  - `submitTask()`：提交任务
  - `start()`：启动线程池
  - `threadFunc()`：线程执行函数

## 构建过程

以下是整个项目文档流程的模板，会逐步根据项目内容进行修改。

### 环境要求
- C++11 或更高版本
- CMake 3.10 或更高版本
- 支持C++11的编译器（如g++ 4.8+）

### 构建步骤

1. 创建构建目录
```bash
mkdir build
cd build
```

2. 配置项目
```bash
cmake ..
```

3. 编译项目
```bash
make
```

4. 运行示例
```bash
./ThreadPool
```

## 使用示例

```cpp
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

```

## 注意事项
1. 任务队列大小默认为1024，可以通过 `setTaskQueMaxSize()` 修改
2. 线程池默认使用固定模式，可以通过 `setMode()` 切换为缓存模式
3. 使用智能指针管理任务生命周期，避免内存泄漏
4. 线程池启动后会自动管理线程的创建和销毁
5. 使用 `submitTask` 提交任务时，会返回 `Result` 对象，通过 `get()` 方法获取结果
6. 使用 `Any` 类存储任意类型的返回值，通过 `cast_<T>()` 方法获取具体类型的值
7. 如果任务提交失败（队列满或超时），返回的 `Result` 对象将无效
