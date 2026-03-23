#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <iostream>

class ThreadPool {
public:
    ThreadPool(size_t threads);
    ~ThreadPool();

    template<class F, class... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>;

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
};

inline ThreadPool::ThreadPool(size_t threads)
    : stop(false)
{
    for(size_t i = 0; i < threads; ++i)
        workers.emplace_back([this] {
            for(;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this]{
                        return this->stop || !this->tasks.empty();
                    });
                    if(this->stop && this->tasks.empty())
                        return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task();
            }
        });
}

inline ThreadPool::~ThreadPool()
{
    stop = true;
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}

template<class F, class... Args>
auto ThreadPool::submit(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type>
{
    using return_type = typename std::invoke_result<F, Args...>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        if(stop)
            throw std::runtime_error("submit on stopped ThreadPool");

        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

// 示例用法
int main() {
    ThreadPool pool(4);

    auto future1 = pool.submit([](int a, int b) { return a + b; }, 1, 2);
    auto future2 = pool.submit([] { std::cout << "Hello from thread pool!" << std::endl; });

    std::cout << "1 + 2 = " << future1.get() << std::endl;
    future2.get(); // 等待输出完成

    return 0;
}