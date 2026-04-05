#pragma once 
#include <memory>
#include <functional>
#include <atomic>
#include <system_error>
#include <sys/epoll.h>
class EventHandler{
public:
    int which_sub=-1;
    virtual ~EventHandler()=default;
    virtual void handle_read()=0;
    virtual void handle_write()=0;
    virtual void handle_error()=0;
    virtual int get_handle()const =0;
    virtual void reset(const std::string& str)=0;
    int status=EPOLLET;
};
class Reactor{
public:
    virtual ~Reactor()=default;
    virtual void register_handler(std::shared_ptr<EventHandler>handler,int event)=0;
    virtual void remove_handler(int handle)=0;
    virtual void run()=0;
    virtual void stop()=0;
};