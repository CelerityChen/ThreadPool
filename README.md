# ThreadPool 线程池项目

## 项目说明
这是一个基于 C++ 实现的线程池项目，提供了固定大小和动态缓存的两种线程池模式。项目使用现代 C++ 特性，包括智能指针、lambda 表达式、条件变量等，实现了一个高效、易用的线程池框架。

## 版本说明

### v1 版本
- 基于 C++11 标准库实现
- 手动实现了 `Any` 类，支持任意类型的存储和类型安全的转换
- 手动实现了 `Semaphore` 信号量，用于线程同步
- 手动实现了 `Result` 类，用于获取任务执行结果
- 支持固定大小和动态扩展两种线程池模式
- 使用智能指针管理线程和任务资源
- 实现了任务队列管理和任务执行结果的异步获取

### v2 版本
- 基于 C++17 标准库实现
- 使用 `std::packaged_task` 和 `std::future` 替代手动实现的 `Result` 类
- 使用标准库的条件变量和互斥量替代手动实现的 `Semaphore`
- 使用模板和 lambda 表达式简化任务提交接口
- 优化了线程池的性能和资源管理
- 改进了错误处理和任务提交机制
- 支持任务提交超时处理

## 版本升级说明

从 v1 升级到 v2 的主要改进：

1. 代码简化：
   - 移除了手动实现的 `Any`、`Semaphore` 和 `Result` 类
   - 使用标准库特性替代自定义实现，提高代码可维护性
   - 简化了任务提交接口，支持直接提交函数和 lambda 表达式

2. 功能增强：
   - 更好的类型安全性和错误处理
   - 支持任务提交超时处理
   - 更高效的资源管理
   - 更灵活的任务提交方式

3. 性能优化：
   - 减少了不必要的内存分配和拷贝
   - 优化了线程同步机制
   - 改进了线程池的动态扩展策略

4. 使用示例：
```cpp
// v1 版本
ThreadPool pool;
pool.start(4);
auto task = std::make_shared<MyTask>(10, 20);
Result result = pool.submitTask(task);
int res = result.get().cast_<int>();

// v2 版本
ThreadPool pool;
pool.start(4);
auto res = pool.submitTask([](int a, int b) { return a + b; }, 10, 20);
int result = res.get();
```

## 配置参数

- `TASK_MAX_THRESHOLD`: 任务队列最大容量（默认1024）
- `THREAD_MAX_SIZE`: 最大线程数（默认10）
- `THREAD_IDLE_MAX_TIME`: 线程最大空闲时间（秒）（默认5）