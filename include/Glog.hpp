#pragma once 
#include <glog/logging.h>
#include <string>
#include <memory>

/**
 * @brief Glog 日志库的 C++ 封装类
 * 
 * 提供简单易用的日志接口，支持不同级别的日志输出
 */
class Logger {
public:
    /**
     * @brief 初始化日志系统
     * @param program_name 程序名称（通常是 argv[0]）
     * @param log_dir 日志目录，为空则输出到 stderr
     * @param to_stderr 是否同时输出到 stderr
     */
    static void Initialize(const char* program_name, 
                          const std::string& log_dir = "/home/chenguo/service/LOG", 
                          bool to_stderr = true) {
        //日志重定向
        google::InitGoogleLogging(program_name);
        //google::InitGoogleLogging(log_name.c_str());
        //设置日志等级
        FLAGS_stderrthreshold=0;
        FLAGS_colorlogtostderr=true;
        FLAGS_logbufsecs=0;
        FLAGS_log_dir=log_dir;
        FLAGS_max_log_size=4;
        //google::SetLogDestination(google::GLOG_WARNING,"");
        //google::SetLogDestination(google::ERROR,"");
        //signal(SIGPIPE,SIG_IGN);
        //std::cout<<"log dir"<<log_dir;
        if (!log_dir.empty()) {
            google::SetLogDestination(google::INFO, (log_dir + "/INFO_").c_str());
            google::SetLogDestination(google::WARNING, (log_dir + "/WARNING_").c_str());
            google::SetLogDestination(google::ERROR, (log_dir + "/ERROR_").c_str());
            google::SetLogDestination(google::FATAL, (log_dir + "/FATAL_").c_str());
        }
        
        // FLAGS_logtostderr = to_stderr;
        // FLAGS_alsologtostderr=to_stderr;
        // FLAGS_colorlogtostderr = true;
        // FLAGS_log_prefix = true;
        // FLAGS_logbufsecs = 0;  // 立即输出
    }

    /**
     * @brief 关闭日志系统
     */
    static void Shutdown() {
        google::ShutdownGoogleLogging();
    }

    /**
     * @brief 设置日志级别
     * @param level 日志级别：INFO, WARNING, ERROR, FATAL
     */
    static void SetLogLevel(google::LogSeverity level) {
        FLAGS_minloglevel = level;
    }

    /**
     * @brief 设置是否输出到 stderr
     * @param enable 是否启用
     */
    static void SetLogToStderr(bool enable) {
        FLAGS_logtostderr = enable;
    }

    /**
     * @brief 设置最大日志文件大小（MB）
     * @param size_mb 文件大小（MB）
     */
    static void SetMaxLogSize(int size_mb) {
        FLAGS_max_log_size = size_mb;
    }
};

/**
 * @brief 日志宏定义，提供更简洁的接口
 */

// 基础日志宏
#define LOG_INFO(message) LOG(INFO) << message
#define LOG_WARNING(message) LOG(WARNING) << message
#define LOG_ERROR(message) LOG(ERROR) << message
#define LOG_FATAL(message) LOG(FATAL) << message

// 带条件的日志宏
#define LOG_IF_INFO(condition, message) LOG_IF(INFO, condition) << message
#define LOG_IF_WARNING(condition, message) LOG_IF(WARNING, condition) << message
#define LOG_IF_ERROR(condition, message) LOG_IF(ERROR, condition) << message

// 定期日志宏（每 N 次记录一次）
#define LOG_EVERY_N_INFO(n, message) LOG_EVERY_N(INFO, n) << message
#define LOG_EVERY_N_WARNING(n, message) LOG_EVERY_N(WARNING, n) << message

// 前 N 次日志宏
#define LOG_FIRST_N_INFO(n, message) LOG_FIRST_N(INFO, n) << message

// 频率限制日志宏（每秒最多记录一次）
#define LOG_EVERY_SECOND_INFO(message) LOG_EVERY_T(INFO, 1) << message

/**
 * @brief 范围日志，用于记录函数的进入和退出
 */
class ScopeLogger {
public:
    ScopeLogger(const std::string& function_name, const std::string& file = "", int line = 0) 
        : function_name_(function_name), file_(file), line_(line) {
        if (!file_.empty()) {
            LOG(INFO) << "[ENTER] " << function_name_ << " at " << file_ << ":" << line_;
        } else {
            LOG(INFO) << "[ENTER] " << function_name_;
        }
    }
    
    ~ScopeLogger() {
        if (!file_.empty()) {
            LOG(INFO) << "[EXIT] " << function_name_ << " at " << file_ << ":" << line_;
        } else {
            LOG(INFO) << "[EXIT] " << function_name_;
        }
    }

private:
    std::string function_name_;
    std::string file_;
    int line_;
};

// 范围日志宏
#define LOG_SCOPE(function_name) ScopeLogger scope_logger_##__LINE__(function_name, __FILE__, __LINE__)
#define LOG_SCOPE_FUNCTION() LOG_SCOPE(__FUNCTION__)

/**
 * @brief 性能计时器
 */
class PerformanceTimer {
public:
    PerformanceTimer(const std::string& operation_name) 
        : operation_name_(operation_name), start_(std::chrono::steady_clock::now()) {}
    
    ~PerformanceTimer() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
        LOG(INFO) << "[PERF] " << operation_name_ << " took " << duration.count() << " ms";
    }

private:
    std::string operation_name_;
    std::chrono::steady_clock::time_point start_;
};

// 性能计时宏
#define PERF_TIMER(operation_name) PerformanceTimer perf_timer_##__LINE__(operation_name)
