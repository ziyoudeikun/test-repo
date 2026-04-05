#pragma once 
#include "ConnectionHandler.hpp"
#include "Reactor.hpp"
#include "SubReactorManager.hpp"
#include "Glog.hpp"
#include <thread>
#include <unordered_map>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
class ConnectionHandler;
class MainReactor:public Reactor{
private:
    int epoll_fd_;
    int server_fd_;
    std::atomic<bool>running_;
    std::thread reactor_thread_;
    std::unordered_map<int,std::shared_ptr<EventHandler>>handlers_;

    static const int MAX_EVENTS=1024;

public:
    MainReactor():epoll_fd_(-1),server_fd_(-1),running_(false){}
    virtual ~MainReactor(){
        LOG(INFO)<<"at ~MainReactor()";
        stop();
        if(epoll_fd_!=-1){
            close(epoll_fd_);
        }
        if(server_fd_!=-1){
            close(server_fd_);
        }
    }
    bool init(int port){
        server_fd_=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,0);
        if(server_fd_==-1){
            LOG(ERROR)<<"Failed to create socket"<<std::endl;
            return false;
        }
        // 设置SO_REUSEADDR
        int opt = 1;
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            LOG(ERROR) << "Failed to set SO_REUSEADDR" << std::endl;
            return false;
        }
        // 绑定地址
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);
        
        if (bind(server_fd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            LOG(ERROR) << "Failed to bind socket" << std::endl;
            return false;
        }
        // 监听
        if (listen(server_fd_, 1024) < 0) {
            LOG(ERROR) << "Failed to listen on socket" << std::endl;
            return false;
        }
        
        // 创建epoll实例
        epoll_fd_ = epoll_create1(0);
        if (epoll_fd_ == -1) {
            LOG(ERROR) << "Failed to create epoll" << std::endl;
            return false;
        }

        epoll_event event{};
        event.events = EPOLLIN;
        event.data.fd = server_fd_;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_fd_, &event) == -1) {
            LOG(ERROR) << "Failed to register serverfd to epoll" << std::endl;
        }

        return true;
    }
    virtual void register_handler(std::shared_ptr<EventHandler>handler,int events)override{
        int handle=handler->get_handle();
        handlers_[handle]=handler;

        epoll_event event{};
        event.events = events;
        event.data.fd = handle;
        
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, handle, &event) == -1) {
            LOG(ERROR) << "Failed to register handler to epoll" << std::endl;
        }
    }
    virtual void remove_handler(int handle) override {
        handlers_.erase(handle);
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, handle, nullptr);
    }
    virtual void run()override{
        running_=true;
        reactor_thread_ = std::thread([this]() {
            epoll_event events[MAX_EVENTS];
            
            while (running_) {
                int num_events = epoll_wait(epoll_fd_, events, MAX_EVENTS, 100);
                
                if (num_events == -1) {
                    if (errno == EINTR) continue;
                    LOG(ERROR) << "epoll_wait error" << std::endl;
                    break;
                }
                
                for (int i = 0; i < num_events; ++i) {
                    int event_fd = events[i].data.fd;
                    
                    if (event_fd == server_fd_) {
                        // 接受新连接
                        LOG(INFO)<<"new connection";
                        accept_connection();
                    } else {
                        auto it = handlers_.find(event_fd);
                        if (it != handlers_.end()) {
                            try {
                                if (events[i].events & EPOLLIN) {
                                    it->second->handle_read();
                                }
                                if (events[i].events & EPOLLOUT) {
                                    it->second->handle_write();
                                }
                                if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                                    it->second->handle_error();
                                }
                            } catch (const std::exception& e) {
                                LOG(ERROR) << "Exception in event handler: " << e.what() << std::endl;
                            }
                        }
                    }
                }
            }
        });
    }
    virtual void stop()override{
        running_=false;
        LOG(INFO)<<"now at stop in main"<<std::endl;
        if(reactor_thread_.joinable()){
            reactor_thread_.join();
        }
    }

private:
    void accept_connection() {
        while (true) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            
            int client_fd = accept4(server_fd_, 
                                   reinterpret_cast<sockaddr*>(&client_addr), 
                                   &client_len, SOCK_NONBLOCK);
            
            if (client_fd == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break; // 没有更多连接
                }
                LOG(ERROR) << "Failed to accept connection" << std::endl;
                continue;
            }
            
            // 将新连接分发给从Reactor
            distribute_connection(client_fd, client_addr);
        }
        //LOG(INFO)<<"new connection end"<<std::endl;
    }
    void distribute_connection(int client_fd, const sockaddr_in& client_addr) {
        // 这里应该实现连接分发逻辑
        // 简单实现：直接创建连接处理器
        auto connection_handler = std::make_shared<ConnectionHandler>(client_fd);
        SubReactorManager::instance().dispatch_connection(std::static_pointer_cast<EventHandler>(connection_handler));
        //LOG(INFO)<<"now smart ptr count is"<<connection_handler.use_count();
        LOG(INFO) << "Accepted connection from " 
                  << inet_ntoa(client_addr.sin_addr) << ":" 
                  << ntohs(client_addr.sin_port) ;
    }
};