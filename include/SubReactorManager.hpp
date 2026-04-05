// SubReactorManager.hpp
#pragma once
#include "Reactor.hpp"
#include "SubReactor.hpp"
#include "Glog.hpp"
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <sys/epoll.h>

class SubReactorManager {
private:
    std::vector<std::unique_ptr<Reactor>> sub_reactors_;
    std::atomic<size_t> current_reactor_;
    std::mutex mutex_;
    static const size_t NUM_SUB_REACTORS = 4; // CPU核心数
    
    SubReactorManager() : current_reactor_(0) {
        initialize();
    }
    
public:

    void register_handler(int index,std::shared_ptr<EventHandler>handler,int events){
        sub_reactors_[index]->register_handler(handler,events);
        //auto ptr=(SubReactor*)sub_reactors_[index].get();
        //LOG(ERROR)<<"now this sub:"<<index<<" size:"<<ptr->get_handler_size();
    }
    void register_handler(int fd,int events,const std::string& str){
        for(auto& temp:sub_reactors_){
            auto ptr=(SubReactor*)temp.get();
            if(auto conn=ptr->get_conn(fd);conn !=std::nullopt){
                auto handle=*conn;
                int index=handle->which_sub;
                handle->reset(str);
                register_handler(index,handle,events);
            }
            LOG(INFO)<<ptr->get_handler_size();
        }
    }
    static SubReactorManager& instance() {
        static SubReactorManager instance;
        return instance;
    }

    void remove_handler(int index,int fd)const{
        sub_reactors_[index]->remove_handler(fd);
    }
    
    void dispatch_connection(std::shared_ptr<EventHandler> handler) {
        size_t index = current_reactor_++ % sub_reactors_.size();
        sub_reactors_[index]->register_handler(handler, EPOLLIN | EPOLLET);
        handler->which_sub=index;
        LOG(INFO)<<"this connection is used by sub :"<<handler->which_sub<<std::endl;
    }
    
    void start_all() {
        for (auto& reactor : sub_reactors_) {
            reactor->run();
        }
    }
    
    void stop_all() {
        for (auto& reactor : sub_reactors_) {
            reactor->stop();
        }
    }
    
private:
    void initialize() {
        for (size_t i = 0; i < NUM_SUB_REACTORS; ++i) {
            sub_reactors_.push_back(std::make_unique<SubReactor>());
        }
    }
};