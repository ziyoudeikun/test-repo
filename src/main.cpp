// main.cpp
#include "../include/MainReactor.hpp"
#include "../include/SubReactorManager.hpp"
#include "../include/Glog.hpp"
#include "../include/GlobalMoudle.hpp"
#include "../include/MysqlConnPoll.hpp"
#include <iostream>
#include <csignal>
#include <atomic>

std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

int main(int argc,char**argv) {
    
    // 初始化日志系统
    Logger::Initialize(argv[0],"../LOG", true);  // 输出到文件和 stderr
    Logger::SetMaxLogSize(10);  // 最大日志文件 10MB
    //Logger::Initialize(argv[0],"/home/chenguo/service/LOG", true);  // 输出到文件和 stderr
    //end

    //initialize the conn mysql poll
    ;
    if (!MysqlConnPoll::getInstance().init("127.0.0.1", "hls_admin", "hls_pwd", "HLS")) {
        LOG(ERROR) << "Failed to initialize connection pool";
    }
    //end


    //initialize the routing system
    auto& router=GlobalMoudle::instance().get_router();
    router.init();
    //end
    
    // 设置信号处理
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    //end

    LOG(INFO)<<"main begin";
    try {
        // 创建主Reactor
        auto main_reactor = std::make_unique<MainReactor>();
        
        // 初始化服务器
        if (!main_reactor->init(9009)) {
            LOG(ERROR) << "Failed to initialize main reactor" << std::endl;
            return 1;
        }
        
        LOG(INFO) << "Server started on port 8080" << std::endl;
        
        // 启动从Reactor
        SubReactorManager::instance().start_all();
        
        // 启动主Reactor
        main_reactor->run();
        LOG(INFO)<<"begin";
        // 主循环
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // 停止服务
        LOG(WARNING) << "Shutting down server..." << std::endl;
        main_reactor->stop();
        LOG(WARNING)<<"main is closed"<<std::endl;
        SubReactorManager::instance().stop_all();
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Server error: " << e.what() << std::endl;
        return 1;
    }
    MysqlConnPoll::getInstance().shutdown();
    LOG(WARNING) << "Server stopped" << std::endl;
    LOG(WARNING)<<"main end()";
    return 0;
}