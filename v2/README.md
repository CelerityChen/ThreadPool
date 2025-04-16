# 线程池实现 (v2)

本项目实现了一个基于 C++ 的线程池，主要特点如下：

## 主要特性

1. 支持两种线程池模式：
   - FIXED 模式：固定线程数量
   - CACHED 模式：动态调整线程数量

2. 使用现代 C++ 特性：
   - 使用 `std::packaged_task` 和 `std::future` 处理任务结果
   - 使用智能指针管理资源
   - 使用 lambda 表达式和函数对象

3. 线程管理：
   - 支持动态创建和回收线程
   - 空闲线程自动回收机制
   - 线程 ID 管理

4. 任务队列：
   - 支持任务队列大小限制
   - 使用条件变量实现任务队列的同步
   - 支持任务提交超时处理

## 使用示例

```cpp
ThreadPool pool;
pool.start(4);  // 启动线程池，初始线程数为4

// 提交任务并获取结果
auto res1 = pool.submitTask(sum1, 10, 20);
std::future<int> res2 = pool.submitTask([](int a, int b) -> int { 
    return a - b; 
}, 30, 10);

// 获取任务结果
std::cout << "Result: " << res1.get() << ", " << res2.get() << std::endl;
```

## 配置参数

- `TASK_MAX_THRESHOLD`: 任务队列最大容量（默认1024）
- `THREAD_MAX_SIZE`: 最大线程数（默认10）
- `THREAD_IDLE_MAX_TIME`: 线程最大空闲时间（秒）（默认5）