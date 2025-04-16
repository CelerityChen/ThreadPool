#include <thread>
#include <future>
#include <iostream>
#include "../include/threadpool.h"

int sum1(int a, int b)
{
    return a + b;
}

int main()
{

    std::packaged_task<int(int, int)> task([](int a, int b) -> int
                                           { return a + b; });

    std::future<int> res = task.get_future();
    std::thread t(std::move(task), 10, 20);
    t.join();
    std::cout << "Result: " << res.get() << std::endl;

    ThreadPool pool;
    pool.start(4);

    auto res1 = pool.submitTask(sum1, 10, 20);
    std::future<int> res2 = pool.submitTask([](int a, int b) -> int
                                            { return a - b; }, 30, 10);
    std::cout << "Result: " << res1.get() << ", " << res2.get() << std::endl;
    return 0;
}