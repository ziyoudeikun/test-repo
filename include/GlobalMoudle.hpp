#pragma once

#include <memory>
#include "Router.hpp"
#include "Glog.hpp"
#include "ThreadPoll.hpp"

class GlobalMoudle{
private:
    GlobalMoudle();
    ~GlobalMoudle();
    std::unique_ptr<Router>router;
    std::unique_ptr<ThreadPool>threadpool;

public:
    static GlobalMoudle& instance();
    Router&get_router();
    ThreadPool&get_threadpool();
};