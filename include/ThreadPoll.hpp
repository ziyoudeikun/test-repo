#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <atomic>

#include "Glog.hpp"

class ThreadPool {
public:
    // 构造函数，创建指定数量的工作线程
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency())
        : stop(false) {
        if (threads == 0) {
            threads = 1; // 至少一个线程
        }
        
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] {
                            return this->stop || !this->tasks.empty();
                        });
                        
                        if (this->stop && this->tasks.empty()) {
                            return;
                        }
                        
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    
                    task();
                }
            });
        }
    }

    // 将任务添加到线程池
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type> {
        
        using return_type = typename std::result_of<F(Args...)>::type;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
            
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            
            // 不允许在停止线程池后添加任务
            if (stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            
            tasks.emplace([task](){ (*task)(); });
        }
        
        condition.notify_one();
        return res;
    }

    // 析构函数，等待所有任务完成
    ~ThreadPool() {
        {
            //LOG(INFO)<<"~poll";
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        
        condition.notify_all();
        
        for (std::thread &worker : workers) {
            worker.join();
        }
    }

    // 获取线程池大小
    size_t size() const {
        return workers.size();
    }

    // 获取等待中的任务数量
    size_t pending() const {
        std::unique_lock<std::mutex> lock(queue_mutex);
        return tasks.size();
    }

private:
    // 工作线程
    std::vector<std::thread> workers;
    
    // 任务队列
    std::queue<std::function<void()>> tasks;
    
    // 同步
    mutable std::mutex queue_mutex;
    std::condition_variable condition;
    
    // 停止标志
    std::atomic<bool> stop;
};
