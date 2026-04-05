// SubReactor.hpp
#pragma once
#include "Reactor.hpp"
#include "GlobalMoudle.hpp"
#include <sys/epoll.h>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <iostream>
#include <mutex>
#include <unistd.h>
#include <optional>
#include <functional>

class SubReactor : public Reactor {
private:
    int epoll_fd_;
    std::atomic<bool> running_;
    std::thread reactor_thread_;
    std::unordered_map<int, std::shared_ptr<EventHandler>> handlers_;
    std::mutex handlers_mutex_;
    
    static const int MAX_EVENTS = 1024;
    
public:
    SubReactor() : epoll_fd_(-1), running_(false) {
        epoll_fd_ = epoll_create1(0);
        if (epoll_fd_ == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to create epoll");
        }
    }
    
    virtual ~SubReactor() {
        stop();
        if (epoll_fd_ != -1) close(epoll_fd_);
    }
    
    virtual void register_handler(std::shared_ptr<EventHandler> handler, int events) override {
        
        epoll_event event{};
        event.events = EPOLLET;
        if(events&EPOLLIN){
            std::cout<<"read"<<std::endl;
            event.events|=EPOLLIN;
        }
        if(events&EPOLLOUT){
            std::cout<<"write"<<std::endl;
            event.events|=EPOLLOUT;
        }
        handler->status=event.events;
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        int handle = handler->get_handle();
        event.data.fd = handle;
        if(auto it=handlers_.find(handle);it==handlers_.end()){
            LOG(INFO)<<"new fd:"<<handle<<std::endl;
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, handle, &event) == -1) {
                std::cerr << "Failed to register handler to sub reactor epoll" << std::endl;
            }
        }else{
            LOG(INFO)<<"old fd:"<<handle<<std::endl;
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, handle, &event) == -1) {
                std::cerr << "Failed to change handler to sub reactor epoll" << std::endl;
            }
        }
        handlers_[handle] = handler;
    }
    
    virtual void remove_handler(int handle) override {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        handlers_.erase(handle);
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, handle, nullptr);
    }
    
    virtual void run() override {
        running_ = true;
        reactor_thread_ = std::thread([this]() {
            epoll_event events[MAX_EVENTS];
            
            while (running_) {
                int num_events = epoll_wait(epoll_fd_, events, MAX_EVENTS, 100);
                
                if (num_events == -1) {
                    if (errno == EINTR) continue;
                    std::cerr << "sub reactor epoll_wait error" << std::endl;
                    break;
                }
                
                for (int i = 0; i < num_events; ++i) {

                    std::function<std::string()>func=[this,&events=events,&i=i]()->std::string{
                        int event_fd = events[i].data.fd;
                    
                        std::shared_ptr<EventHandler> handler;
                        {
                            std::lock_guard<std::mutex> lock(handlers_mutex_);
                            auto it = handlers_.find(event_fd);
                            if (it != handlers_.end()) {
                                handler = it->second;
                            }
                        }
                        
                        if (handler) {
                            try {
                                if (events[i].events & EPOLLIN) {
                                    std::cout<<"this is dealing with read"<<" fd="<<handler->get_handle()<<std::endl;
                                    handler->handle_read();
                                }
                                if (events[i].events & EPOLLOUT) {
                                    handler->handle_write();
                                }
                                if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                                    handler->handle_error();
                                    remove_handler(event_fd);
                                }
                            } catch (const std::exception& e) {
                                LOG(INFO) << "Exception in sub reactor handler: " << e.what() << std::endl;
                                remove_handler(event_fd);
                            }
                        }
                        return "all task is ending";
                    };

                    GlobalMoudle::instance().get_threadpool().enqueue(func).get();





                    // int event_fd = events[i].data.fd;
                    
                    // std::shared_ptr<EventHandler> handler;
                    // {
                    //     std::lock_guard<std::mutex> lock(handlers_mutex_);
                    //     auto it = handlers_.find(event_fd);
                    //     if (it != handlers_.end()) {
                    //         handler = it->second;
                    //     }
                    // }
                    
                    // if (handler) {
                    //     try {
                    //         if (events[i].events & EPOLLIN) {
                    //             std::cout<<"this is dealing with read"<<" fd="<<handler->get_handle()<<std::endl;
                    //             handler->handle_read();
                    //         }
                    //         if (events[i].events & EPOLLOUT) {
                    //             handler->handle_write();
                    //         }
                    //         if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                    //             handler->handle_error();
                    //             remove_handler(event_fd);
                    //         }
                    //     } catch (const std::exception& e) {
                    //         LOG(INFO) << "Exception in sub reactor handler: " << e.what() << std::endl;
                    //         remove_handler(event_fd);
                    //     }
                    // }
                }
            }
        });
    }
    
    virtual void stop() override {
        running_ = false;
        if (reactor_thread_.joinable()) {
            reactor_thread_.join();
        }
    }

    int get_handler_size()const{
        return handlers_.size();
    }

    std::optional<std::shared_ptr<EventHandler>> get_conn(int fd)const{
        if(auto conn=handlers_.find(fd);conn!=handlers_.end()){
            return handlers_.at(fd);
        }else{
            return std::nullopt;
        }
    }
};