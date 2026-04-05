// ConnectionHandler.hpp
#pragma once
#include "Reactor.hpp"
#include "HttpParser.hpp"
#include "SubReactorManager.hpp"
#include "GlobalMoudle.hpp"
#include "HttpParser.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

/**
 * @brief 解析HLS .ts文件路径，提取前缀和序号
 * @param tsPath .ts文件的完整路径
 * @param prefix [输出] 提取的前缀部分（不包含序号和扩展名）
 * @param number [输出] 提取的序号部分
 * @return bool 是否成功解析
 */
inline bool parseTsFilePath(const std::string& tsPath, std::string& prefix, std::string& number) {
    // 检查是否以.ts结尾
    if (tsPath.length() < 3 || tsPath.substr(tsPath.length() - 3) != ".ts") {
        return false;
    }
    
    // 去除.ts扩展名
    std::string pathWithoutExt = tsPath.substr(0, tsPath.length() - 3);
    
    // 查找最后一个下划线
    size_t lastUnderscore = pathWithoutExt.find_last_of('_');
    
    if (lastUnderscore == std::string::npos) {
        // 没有下划线，无法提取序号
        return false;
    }
    
    // 提取前缀部分（下划线之前的所有内容）
    prefix = pathWithoutExt.substr(0, lastUnderscore);
    
    // 提取序号部分（下划线之后的内容）
    number = pathWithoutExt.substr(lastUnderscore + 1);
    
    // 检查序号是否只包含数字
    for (char c : number) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 使用正则表达式解析.ts文件路径
 * @param tsPath .ts文件的完整路径
 * @param prefix [输出] 提取的前缀部分
 * @param number [输出] 提取的序号部分
 * @return bool 是否成功解析
 */
inline bool parseTsFilePathRegex(const std::string& tsPath, std::string& prefix, std::string& number) {
    // 正则表达式匹配 pattern: (.*/)?(.*?)_(\d+)\.ts
    // 解释:
    // (.*/)?         可选的目录部分
    // (.*?)          非贪婪匹配文件名前缀
    // _(\d+)         下划线后的数字序列
    // \.ts           .ts扩展名
    std::regex pattern(R"((.*/)?(.*?)_(\d+)\.ts)");
    std::smatch matches;
    
    if (std::regex_match(tsPath, matches, pattern)) {
        if (matches.size() == 4) {
            // 组合目录和文件名前缀
            std::string dir = matches[1].str();
            std::string filePrefix = matches[2].str();
            
            prefix = dir + filePrefix;
            number = matches[3].str();
            
            return true;
        }
    }
    
    return false;
}
class ConnectionHandler : public EventHandler,public std::enable_shared_from_this<ConnectionHandler> {
private:
    int client_fd_;
    std::vector<char> read_buffer_;
    std::vector<char> write_buffer_;
    std::string result;
    static const size_t BUFFER_SIZE = 4096;
public:
    explicit ConnectionHandler(int client_fd) : client_fd_(client_fd) {
        //LOG(ERROR)<<"client "<<client_fd<<"is made";
        read_buffer_.resize(BUFFER_SIZE);
    }
    
    virtual ~ConnectionHandler() {
        if (client_fd_ != -1) {
            close(client_fd_);
        }
        std::cout<<"now fd="<<client_fd_<<" is ~ConnectionHandler()"<<std::endl;
    }
    
    virtual int get_handle() const override {
        return client_fd_;
    }
    
    virtual void handle_read() override {
        ssize_t bytes_read = recv(client_fd_, read_buffer_.data(), read_buffer_.size(), 0);
        
        if (bytes_read > 0) {
            // 处理接收到的数据
            std::string request(read_buffer_.data(), bytes_read);
            std::cout << "Received: " << request << std::endl;

            //parser http request
            HttpParser parser;
            parser.parse(request);
            // 获取解析结果
            //LOG(INFO)<< "\n=== Parsing Results ===" << std::endl;
            //LOG(INFO)<< "Method: " << parser.getMethod() << std::endl;
            LOG(WARNING)<< "URL: " << parser.getUrl() << std::endl;
            //LOG(INFO)<< "body: "<<parser.getBody();
            auto&temp=parser.getHeaders();
            //LOG(INFO)<< "size: "<<temp.size();
            //auto it=temp.find("Authorization");
            //if(it!=temp.end()){
                //LOG(INFO)<< "other data:"<<it->second;
            //}

            
            // 简单的echo响应
            //std::string response = "Echo: " + request;
            // std::string response = "HTTP/1.1 200 OK\r\n"
            // "Content-Type: text/html; charset=utf-8\r\n"
            // "Content-Length: 25\r\n"
            // "Connection: keep-alive\r\n"
            // "\r\n"
            // "<h1>Hello, World!</h1>";
            Router::Request req(parser.getMethod(),parser.getUrl(),parser.getBody(),write_buffer_);
            req.headers=std::move(temp);
            req.otherData=std::to_string(client_fd_);
            Router::Response res;
            LOG(INFO)<<req.method;
            LOG(INFO)<<req.path;
            if(req.path.size()>3&&req.path.substr(req.path.size()-3)==".ts"){
                req.queryParams["seg"]=req.path.substr(req.path.size()-6,3);
                req.path=req.path.substr(0,req.path.size()-6);
                LOG(INFO)<<"it is a ts file,and the path is "<<req.path;
                LOG(INFO)<<"its seg num is "<<req.queryParams["seg"];
            }
            LOG(INFO)<<GlobalMoudle::instance().get_router().handleRequest(req,res);
            //write_buffer_.assign(res.body.begin(),res.body.end());
            LOG(WARNING)<<write_buffer_.size();
            // 注册写事件
            // 在实际应用中，这里应该通知Reactor注册写事件
            SubReactorManager::instance().register_handler(which_sub,shared_from_this(),EPOLLIN|EPOLLOUT);
            //handle_write();
            
        } else if (bytes_read == 0) {
            // 连接关闭
            //std::cout << "Client disconnected" << std::endl;
            handle_error();
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cerr << "Read error" << std::endl;
                handle_error();
            }
        }
    }
  
    virtual void handle_write() override {
        LOG(INFO) << "handle_write called, buffer size: " << write_buffer_.size();
        
        if (write_buffer_.empty()) {
            LOG(INFO) << "Write buffer is empty, disabling write event";
            SubReactorManager::instance().register_handler(which_sub, shared_from_this(), EPOLLIN);
            return;
        }
        
        // 循环发送直到缓冲区为空或发生EAGAIN
        while (!write_buffer_.empty()) {
            ssize_t bytes_sent = send(client_fd_, write_buffer_.data(), 
                                    write_buffer_.size(), MSG_NOSIGNAL);
            
            LOG(INFO) << "Attempted to send " << write_buffer_.size() 
                    << " bytes, actually sent: " << bytes_sent;
            
            if (bytes_sent > 0) {
                // 成功发送了部分或全部数据
                write_buffer_.erase(write_buffer_.begin(), 
                                write_buffer_.begin() + bytes_sent);
                
                LOG(INFO) << "Remaining buffer size: " << write_buffer_.size();
                
                // 如果还有数据，继续留在while循环中发送
            } 
            else if (bytes_sent == 0) {
                // 对端关闭连接
                LOG(ERROR) << "Peer closed connection during write";
                handle_error();
                return;
            }
            else {
                // 发送出错
                int error_code = errno;
                
                if (error_code == EAGAIN || error_code == EWOULDBLOCK) {
                    // 发送缓冲区满，需要等待下次可写事件
                    LOG(INFO) << "Send buffer full (EAGAIN/EWOULDBLOCK), waiting for next write event";
                    
                    // 保持写事件监听，等待下次可写
                    SubReactorManager::instance().register_handler(which_sub, 
                        shared_from_this(), EPOLLIN | EPOLLOUT);
                    return;
                }
                else {
                    // 其他错误
                    LOG(ERROR) << "Send error: " << strerror(error_code);
                    handle_error();
                    return;
                }
            }
        }
        
        // 所有数据都已发送完成
        LOG(INFO) << "All data sent successfully";
        
        // 禁用写事件监听，只监听读事件
        SubReactorManager::instance().register_handler(which_sub, 
            shared_from_this(), EPOLLIN);
    }

    // virtual void handle_write() override {
    //     // std::cout<<"now write buffsize is :"<<write_buffer_.size()<<std::endl;
    //     // std::string s;
    //     // for(size_t i=0;i<write_buffer_.size();++i){
    //     //    s+=write_buffer_[i];
    //     // }
    //     // std::cout<<s<<std::endl;
    //     //LOG(INFO)<<"info end";

    //     if (!write_buffer_.empty()) {
    //         ssize_t bytes_sent = send(client_fd_, write_buffer_.data(), write_buffer_.size(), MSG_NOSIGNAL);
    //         LOG(INFO)<<"now send byte:"<<bytes_sent;
    //         if (bytes_sent > 0) {
    //             // 移除已发送的数据
    //             if (static_cast<size_t>(bytes_sent) < write_buffer_.size()) {
    //                 write_buffer_.erase(write_buffer_.begin(), 
    //                                   write_buffer_.begin() + bytes_sent);
    //             } else {
    //                 write_buffer_.clear();
    //             }
    //         } else {
    //             if (errno != EAGAIN && errno != EWOULDBLOCK) {
    //                 std::cerr << "Write error" << std::endl;
    //                 handle_error();
    //             }
    //         }
    //     }else{
    //         //LOG(INFO)<<"now all data has been sent,close write work"<<std::endl;
    //         //LOG(INFO)<<"now i will close write listen";
    //         //SubReactorManager::instance().register_handler(which_sub,shared_from_this(),EPOLLIN);
    //         //disabled_write_event();
    //     }
    //     //std::cout<<"now send end"<<std::endl;
    //     //LOG(INFO)<<"now all data has been sent,close write work"<<std::endl;
    //     //LOG(INFO)<<"now i will close write listen";
    //     SubReactorManager::instance().register_handler(which_sub,shared_from_this(),EPOLLIN);
    //     //disabled_write_event();
    // }
    
    virtual void handle_error() override {
        SubReactorManager::instance().remove_handler(which_sub,client_fd_);
        if (client_fd_ != -1) {
            close(client_fd_);
            client_fd_ = -1;
        }
        //std::cout<<handler_
    }
    virtual void reset(const std::string&str)override{
        write_buffer_.assign(str.begin(),str.end());
    }
};