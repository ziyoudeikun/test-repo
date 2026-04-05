 #include "../include/GlobalMoudle.hpp"
using namespace std;
GlobalMoudle::GlobalMoudle():router(make_unique<Router>()),
    threadpool(make_unique<ThreadPool>()){

}
GlobalMoudle::~GlobalMoudle(){
    LOG(INFO)<<"just only one class is ~()";
    Logger::Shutdown();
}
GlobalMoudle& GlobalMoudle::instance(){
    static GlobalMoudle instance;
    return instance;
}
Router& GlobalMoudle::get_router(){
    return *router;
}
ThreadPool& GlobalMoudle::get_threadpool(){
    return *threadpool;
}