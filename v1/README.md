# C++11 线程池实现

这是一个基于 C++11 标准库实现的线程池项目，提供了灵活的任务调度和线程管理功能。

## 主要特性

- 支持固定大小和动态扩展两种线程池模式
- 实现了类似 C++17 的 `std::any` 的 `Any` 类，支持任意类型的存储和类型安全的转换
- 实现了 `Semaphore` 信号量，用于线程同步
- 支持任务队列管理，可设置最大任务数量
- 支持任务执行结果的异步获取
- 支持线程池的动态扩展和收缩

## 项目结构

```
v1/
├── include/
│   └── threadpool.h    # 线程池头文件
└── src/
    ├── threadpool.cpp  # 线程池实现
    └── main.cpp        # 示例代码
```

## 核心组件

### 1. Any 类
实现了类似 C++17 的 `std::any` 功能，使用类型擦除技术支持存储任意类型的数据：
```cpp
Any any = 42;           // 存储 int
int value = any.cast_<int>();  // 安全地获取值
```

### 2. Semaphore 信号量
实现了基本的信号量功能，用于线程同步：
```cpp
Semaphore sem(1);  // 初始资源数为1
sem.wait();        // 获取资源
sem.post();        // 释放资源
```

### 3. Task 类
- 抽象基类，用于定义任务接口
- 主要成员：
  - `result_`: 指向任务执行结果的指针
- 主要方法：
  - `run()`: 纯虚函数，需要由用户继承实现具体的任务逻辑
  - `exec()`: 执行任务并设置结果
  - `setResult()`: 设置结果对象

### 4. Result 类
- 用于接收和存储任务执行结果
- 主要成员：
  - `any_`: 存储任意类型的任务结果
  - `sem_`: 信号量，用于同步任务执行
  - `task_`: 关联的任务对象
  - `isValid_`: 结果有效性标志
- 主要方法：
  - `setVal()`: 设置任务结果
  - `get()`: 获取任务结果

### 5. Thread 类
- 线程封装类
- 主要成员：
  - `threadFunc_`: 线程执行函数
  - `threadId_`: 线程ID
  - `generateId_`: 静态成员，用于生成线程ID
- 主要方法：
  - `start()`: 启动线程
  - `getThreadId()`: 获取线程ID

### 6. ThreadPool 类
- 线程池核心类
- 主要成员：
  - `threads_`: 线程集合，使用 `std::unordered_map` 管理
  - `taskQue_`: 任务队列，使用 `std::queue` 存储
  - `taskSize_`: 当前任务数量
  - `maxTaskQueSize_`: 最大任务队列大小
  - `poolMode_`: 线程池模式（固定/动态）
  - `initThreadSize_`: 初始线程数量
  - `maxThreadSize_`: 最大线程数量
  - `curThreadSize_`: 当前线程数量
  - `idleThreadSize_`: 空闲线程数量
- 主要方法：
  - `setMode()`: 设置线程池模式
  - `setThreadMaxSize()`: 设置最大线程数
  - `setTaskQueMaxSize()`: 设置任务队列大小
  - `submitTask()`: 提交任务
  - `start()`: 启动线程池
  - `threadFunc()`: 线程执行函数
  - `checkRunningState()`: 检查线程池运行状态

主要功能：
- 设置线程池模式
- 设置最大线程数
- 设置任务队列大小
- 提交任务并获取结果
- 自动管理线程生命周期

## 使用示例

```cpp
// 创建线程池
ThreadPool pool;
pool.setMode(PoolMode::MODE_CACHED);
pool.start(4);  // 初始4个线程

// 定义任务
class MyTask : public Task {
public:
    Any run() override {
        // 任务逻辑
        return result;
    }
};

// 提交任务
Result result = pool.submitTask(std::make_shared<MyTask>());
// 获取结果
Any value = result.get();
```

## 编译运行

### 使用CMake构建项目

1. 创建构建目录并进入：
```bash
mkdir build
cd build
```

2. 配置项目：
```bash
cmake ..
```

3. 编译项目：
```bash
make
```

4. 运行程序：
```bash
cd bin
./ThreadPool
```

### 构建选项

- 默认使用C++11标准
- 可执行文件将生成在 `build/bin` 目录下
- 支持在Linux和Windows系统上构建

### 依赖要求

- CMake 3.10 或更高版本
- C++11 兼容的编译器（如 g++ 4.8+ 或 MSVC 2013+）
- 支持多线程的操作系统

## 注意事项

- 需要 C++11 或更高版本的编译器支持
- 线程池在析构时会自动等待所有任务完成
- 任务队列满时会阻塞提交，直到有空间可用
- 动态模式下，空闲线程超过一定时间会被回收

## 性能考虑

- 使用 `std::unique_ptr` 管理线程资源
- 使用 `std::shared_ptr` 管理任务对象
- 使用条件变量实现高效的任务调度
- 使用原子操作保证线程安全 