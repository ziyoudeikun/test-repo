#pragma once

#include "MySqlCppConn.hpp"
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <thread>
#include <chrono>

class MysqlConnPoll {
public:
    // 使用现代C++的删除函数和默认函数
    MysqlConnPoll(const MysqlConnPoll&) = delete;
    MysqlConnPoll& operator=(const MysqlConnPoll&) = delete;
    
    // 移动构造和移动赋值
    MysqlConnPoll(MysqlConnPoll&&) = delete;
    MysqlConnPoll& operator=(MysqlConnPoll&&) = delete;

    // 单例模式 - 使用现代C++的线程安全实现
    static MysqlConnPoll& getInstance() {
        static MysqlConnPoll instance;
        return instance;
    }

    // 初始化连接池
    bool init(const std::string& host, const std::string& user, 
              const std::string& password, const std::string& database,
              int port = 3306, size_t poolSize = 10, size_t maxPoolSize = 20) {
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (initialized_) {
            return true;
        }

        // 存储连接参数
        host_ = host;
        user_ = user;
        password_ = password;
        database_ = database;
        port_ = port;
        poolSize_ = poolSize;
        maxPoolSize_ = maxPoolSize;

        // 创建初始连接
        for (size_t i = 0; i < poolSize; ++i) {
            auto conn = createConnection();
            if (conn && conn->isConnected()) {
                availableConnections_.push(std::move(conn));
            } else {
                return false;
            }
        }

        initialized_ = true;
        return true;
    }

    // 获取连接 - 使用现代C++的智能指针
    std::unique_ptr<MysqlConn> getConnection(int timeoutMs = 5000) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (!initialized_) {
            return nullptr;
        }

        // 等待可用连接或超时
        if (availableConnections_.empty()) {
            // 如果还可以创建新连接
            if (totalConnections_ < maxPoolSize_) {
                auto newConn = createConnection();
                if (newConn && newConn->isConnected()) {
                    ++totalConnections_;
                    return newConn;
                }
            }
            
            // 等待现有连接释放
            if (timeoutMs > 0) {
                auto status = condition_.wait_for(lock, 
                    std::chrono::milliseconds(timeoutMs));
                if (status == std::cv_status::timeout) {
                    return nullptr;
                }
            } else {
                condition_.wait(lock);
            }
        }

        if (availableConnections_.empty()) {
            return nullptr;
        }

        auto conn = std::move(availableConnections_.front());
        availableConnections_.pop();
        return conn;
    }

    // 归还连接 - 使用移动语义
    void returnConnection(std::unique_ptr<MysqlConn> conn) {
        if (!conn || !conn->isConnected()) {
            // 连接无效，创建新连接替换
            conn = createConnection();
            if (!conn) {
                return;
            }
        }

        std::lock_guard<std::mutex> lock(mutex_);
        availableConnections_.push(std::move(conn));
        condition_.notify_one();
    }

    // 获取连接池状态 - 使用现代C++的结构化绑定
    auto getPoolStatus() {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::make_tuple(
            availableConnections_.size(),
            totalConnections_.load() - availableConnections_.size(),
            totalConnections_.load(),
            poolSize_,
            maxPoolSize_
        );
    }

    // 关闭连接池
    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!availableConnections_.empty()) {
            availableConnections_.pop();
        }
        initialized_ = false;
        totalConnections_ = 0;
    }

    // 健康检查
    void healthCheck() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::queue<std::unique_ptr<MysqlConn>> validConnections;
        size_t validCount = 0;

        while (!availableConnections_.empty()) {
            auto conn = std::move(availableConnections_.front());
            availableConnections_.pop();

            if (conn && conn->isConnected()) {
                // 简单测试连接是否有效
                try {
                    conn->execute("SELECT 1");
                    validConnections.push(std::move(conn));
                    ++validCount;
                } catch (...) {
                    // 连接失效，创建新连接替换
                    auto newConn = createConnection();
                    if (newConn) {
                        validConnections.push(std::move(newConn));
                        ++validCount;
                    }
                }
            } else {
                // 创建新连接替换失效连接
                auto newConn = createConnection();
                if (newConn) {
                    validConnections.push(std::move(newConn));
                    ++validCount;
                }
            }
        }

        // 补充连接数量到初始大小
        while (validCount < poolSize_) {
            auto newConn = createConnection();
            if (newConn) {
                validConnections.push(std::move(newConn));
                ++validCount;
            } else {
                break;
            }
        }

        availableConnections_ = std::move(validConnections);
        totalConnections_ = validCount;
    }

private:
    // 私有构造函数
    MysqlConnPoll() = default;

    // 创建新连接 - 使用现代C++的make_unique
    std::unique_ptr<MysqlConn> createConnection() {
        auto conn = std::make_unique<MysqlConn>();
        if (conn->connect(host_, user_, password_, database_, port_)) {
            ++totalConnections_;
            return conn;
        }
        return nullptr;
    }

private:
    // 连接池存储 - 使用现代C++的智能指针和容器
    std::queue<std::unique_ptr<MysqlConn>> availableConnections_;
    
    // 同步原语 - 使用现代C++的线程安全组件
    std::mutex mutex_;
    std::condition_variable condition_;
    
    // 连接池配置
    std::string host_;
    std::string user_;
    std::string password_;
    std::string database_;
    int port_ = 3306;
    size_t poolSize_ = 10;
    size_t maxPoolSize_ = 20;
    
    // 状态变量 - 使用原子操作
    std::atomic<size_t> totalConnections_{0};
    std::atomic<bool> initialized_{false};
};

// 使用RAII的连接守卫类
class ConnectionGuard {
public:
    ConnectionGuard(std::unique_ptr<MysqlConn> conn, MysqlConnPoll& pool)
        : conn_(std::move(conn)), pool_(pool) {}
    
    ~ConnectionGuard() {
        if (conn_) {
            pool_.returnConnection(std::move(conn_));
        }
    }
    
    // 获取连接指针
    MysqlConn* operator->() const { return conn_.get(); }
    MysqlConn* get() const { return conn_.get(); }
    
    // 禁止拷贝
    ConnectionGuard(const ConnectionGuard&) = delete;
    ConnectionGuard& operator=(const ConnectionGuard&) = delete;
    
    // 允许移动
    ConnectionGuard(ConnectionGuard&&) = default;
    ConnectionGuard& operator=(ConnectionGuard&&) = default;

private:
    std::unique_ptr<MysqlConn> conn_;
    MysqlConnPoll& pool_;
};