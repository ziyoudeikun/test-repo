#pragma once 

#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <fstream>
#include "JsonParse.hpp"
#include "MysqlConnPoll.hpp"
#include <iomanip>
//class KafkaConsumer;
#include <random>
#include <sstream>
#include <iomanip>
#include <uuid/uuid.h>
#include <filesystem>

enum class ContentType {
    TEXT,
    JSON,
    HTML,
    XML,
    TS,
    M3U8,
    KEY
};

//read key file from path to buffer
inline bool read_key_file_to_buffer(const std::string&path,
    const std::string&key_file_name,std::vector<char>&buffer_){
    namespace fs=std::filesystem;
    if(!fs::exists(path)||!fs::is_directory(path)){
        LOG(ERROR)<<"path is not valid"<<" path:"<<path;
        return false;
    }
    // 遍历文件夹
    std::string temp="[";
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().filename() == key_file_name) {
            std::string filePath = entry.path().string();
            
            try {
                // 打开并读取文件
                std::ifstream file(filePath);
                if (!file.is_open()) {
                    LOG(ERROR) << "警告：无法打开文件: " << filePath << std::endl;
                    continue;
                }
                
                // 读取文件内容
                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string content = buffer.str();
                file.close();
                

                //LOG(INFO)<<content;
                //InfoJson jsonobj;
                temp+=content;
                temp+=",";
                // 解析JSON
                // json jsonObj;
                // try {
                //     jsonObj = json::parse(content);
                    
                //     // 可选：添加文件路径信息到JSON对象
                //     jsonObj["_file_path"] = entry.path().string();
                //     jsonObj["_folder_path"] = entry.path().parent_path().string();
                    
                //     // 添加到数组
                //     jsonArray.push_back(jsonObj);
                    
                // } catch (const json::parse_error& e) {
                //     std::cerr << "警告：JSON解析错误 (" << filePath << "): " << e.what() << std::endl;
                //     continue;
                // }
                
            } catch (const std::exception& e) {
                std::cerr << "警告：读取文件时出错 (" << filePath << "): " << e.what() << std::endl;
                continue;
            }
        }
    }
    
    // 构建最终JSON数组
    //json result = jsonArray;
    temp[temp.size()-1]=']';
    buffer_.assign(temp.begin(),temp.end());
    return true;
}

//一次性读取整个文件
inline bool read_file_to_buffer(const std::string& filename,
                    std::vector<char>&write_buffer_) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOG(ERROR) << "Failed to open file: " << filename << std::endl;
        return false;
    }
    
    // 获取文件大小
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // 调整缓冲区大小并读取
    write_buffer_.resize(size);
    if (!file.read(write_buffer_.data(), size)) {
        LOG(ERROR) << "Failed to read file: " << filename << std::endl;
        return false;
    }
    
    LOG(INFO) << "Read " << size << " bytes from " << filename << std::endl;
    file.close();
    return true;
}



inline std::string generateHttpResponse(int datalen,std::vector<char>&write_buffer_, 
    ContentType type = ContentType::HTML, int statusCode = 200) {
    std::ostringstream response;
    
    // 状态码描述
    std::string statusText;
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 201: statusText = "Created"; break;
        case 400: statusText = "Bad Request"; break;
        case 404: statusText = "Not Found"; break;
        case 500: statusText = "Internal Server Error"; break;
        default: statusText = "OK";
    }
    
    // 内容类型
    std::string contentType;
    switch (type) {
        case ContentType::KEY:
            contentType = "video/octet-stream";
            break;
        case ContentType::TS:
            contentType = "video/mp2t";
            break;
        case ContentType::JSON:
            contentType = "application/json";
            break;
        case ContentType::M3U8:
            contentType = "application/x-mpegURL";
            break;
        case ContentType::HTML:
            contentType = "text/html";
            break;
        case ContentType::XML:
            contentType = "application/xml";
            break;
        case ContentType::TEXT:
        default:
            contentType = "text/plain";
    }
    
    // HTTP 响应头
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    // 添加CORS头（关键部分）
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Methods: GET, POST, OPTIONS, HEAD\r\n";
    response << "Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept\r\n";
    response << "Access-Control-Expose-Headers: Content-Length, Content-Range\r\n";
    response << "Access-Control-Max-Age: 86400\r\n";
    response << "Accept-Ranges: bytes\r\n";  // 关键！必须添加这一行
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << datalen << "\r\n";
    response << "Connection: keep-alive\r\n";
    response << "Server: CustomServer/1.0\r\n";
    response << "Date: " << "Mon, 01 Jan 2024 12:00:00 GMT\r\n";
    response << "\r\n";
    LOG(INFO)<<"now head len:"<<response.str();
    for(auto ch:write_buffer_){
        response<<ch;
    }
    std::string result = response.str();
    write_buffer_.reserve(result.size());
    write_buffer_.assign(result.begin(),result.end());
    //LOG(INFO)<<"write_buffer.size():"<<write_buffer_.size();
    //LOG(INFO)<<"now all len:"<<result.size();
    //LOG(INFO)<<"now head compute len:"<<result.size()-datalen;
    
    //return result;
    return "";
}
/**
 * 简单HTTP路由器
 * 支持GET、POST、PUT、DELETE等方法的路由注册和分发
 */
class Router {
public:
    
    // 请求上下文，包含请求信息
    struct Request {
        std::string method;
        std::string path;
        std::string body;
        std::unordered_map<std::string, std::string> headers;
        std::unordered_map<std::string, std::string> queryParams;
        std::string otherData;
        std::vector<char>& buffer;
        
        Request(const std::string& m, const std::string& p, const std::string& b , std::vector<char>& buf)
            : method(m), path(p), body(b) ,buffer(buf) {}
    };

    // 响应上下文，用于构建响应
    struct Response {
        int statusCode = 200;
        std::string body;
        std::unordered_map<std::string, std::string> headers;
        
        void setHeader(const std::string& key, const std::string& value) {
            headers[key] = value;
        }
        
        void setStatus(int code) {
            statusCode = code;
        }
        
        void write(const std::string& data) {
            body += data;
        }
    };

    // 处理器函数类型
    using Handler = std::function<void(const Request&, Response&)>;
    //using Handler = std::function<void()>;

private:
    // 路由表结构：method -> path -> handler
    std::unordered_map<std::string, 
        std::unordered_map<std::string, Handler>> routes_;
    std::unordered_map<std::string,std::tuple<int,std::string,std::string>> token_record_;
    //producer
    //count of grab-function
    static inline int grab_function_count;

    static inline int random_id;

public:
    /**
     * 注册路由处理器
     * @param method HTTP方法 (GET, POST, PUT, DELETE等)
     * @param path 请求路径
     * @param handler 处理函数
     */
    void addRoute(const std::string& method, const std::string& path, Handler handler) {
        routes_[method][path] = handler;
    }

    /**
     * 快捷方法：注册GET路由
     */
    void get(const std::string& path, Handler handler) {
        addRoute("GET", path, handler);
    }

    /**
     * 快捷方法：注册POST路由
     */
    void post(const std::string& path, Handler handler) {
        addRoute("POST", path, handler);
    }

    /**
     * 快捷方法：注册PUT路由
     */
    void put(const std::string& path, Handler handler) {
        addRoute("PUT", path, handler);
    }

    /**
     * 快捷方法：注册DELETE路由
     */
    void del(const std::string& path, Handler handler) {
        addRoute("DELETE", path, handler);
    }

    /**
     * 处理HTTP请求
     * @param req 请求对象
     * @param res 响应对象
     * @return true 找到对应的处理器，false 未找到路由
     */
    bool handleRequest(const Request& req, Response& res) {
        //LOG(INFO)<<req.method;
        //for(auto&[k,v]:routes_){
        //    //LOG(INFO)<<k;
        //}
        auto methodIt = routes_.find(req.method);
        if (methodIt == routes_.end()) {
            return false;
        }
        //LOG(INFO)<<methodIt->first;
        auto pathIt = methodIt->second.find(req.path);
        if (pathIt == methodIt->second.end()) {
            return false;
        }

        // 调用对应的处理器
        pathIt->second(req, res);
        return true;
    }

    /**
     * 处理HTTP请求（简化版本）
     * @param method HTTP方法
     * @param path 请求路径
     * @param body 请求体
     * @return 响应对象
     */
    Response handleRequest(const std::string& method, const std::string& path, 
                          const std::string& body = "") {
        std::vector<char>buf;
        Request req(method, path, body, buf);
        Response res;
        
        if (!handleRequest(req, res)) {
            res.setStatus(404);
            res.write("404 Not Found");
        }
        
        return res;
    }

    /**
     * 检查路由是否存在
     */
    bool routeExists(const std::string& method, const std::string& path) const {
        auto methodIt = routes_.find(method);
        if (methodIt == routes_.end()) return false;
        
        return methodIt->second.find(path) != methodIt->second.end();
    }

    /**
     * 获取注册的路由数量
     */
    size_t getRouteCount() const {
        size_t count = 0;
        for (const auto& methodPair : routes_) {
            count += methodPair.second.size();
        }
        return count;
    }

    /**
     * 清空所有路由
     */
    void clear() {
        routes_.clear();
    }

    //get info map
    auto& get_record()const{
        return token_record_;
    }

    void init(){
        grab_function_count=0;
        // 使用lambda表达式注册路由
            //if(parser.getUrl()=="/favicon.ico"){
            //    stdfilename="../source/favicon.ico";
            //}

        //test the api press
        get("/", [](const Request&req,Response&res) {
            std::string filename="../source/login.html";
            //std::string filename="../source/index2.html";
            std::vector<char>&buffer=req.buffer;
            read_file_to_buffer(filename,buffer);
            res.write(generateHttpResponse(buffer.size(),buffer));
        });

        get("/login.html", [](const Request&req,Response&res) {
            std::string filename="../source/login.html";
            //std::string filename="../source/index2.html";
            std::vector<char>&buffer=req.buffer;
            read_file_to_buffer(filename,buffer);
            res.write(generateHttpResponse(buffer.size(),buffer));
        });

        get("/index.html", [](const Request&req,Response&res) {
            std::string filename="../source/index.html";
            //std::string filename="../source/index2.html";
            std::vector<char>&buffer=req.buffer;
            read_file_to_buffer(filename,buffer);
            res.write(generateHttpResponse(buffer.size(),buffer));
        });

        get("/cors.html", [](const Request&req,Response&res) {
            std::string filename="../source/cors.html";
            //std::string filename="../source/index2.html";
            std::vector<char>&buffer=req.buffer;
            read_file_to_buffer(filename,buffer);
            res.write(generateHttpResponse(buffer.size(),buffer));
        });

        get("/favicon.ico", [](const Request&req,Response&res) {
            std::string filename="../source/favicon.ico";
            std::vector<char>&buffer=req.buffer;
            read_file_to_buffer(filename,buffer);
            res.write(generateHttpResponse(buffer.size(),buffer));
        });

        get("/tsparse.html",[](const Request&req,Response&res){
            std::string filename="../source/tsparse.html";
            std::vector<char>&buffer=req.buffer;
            read_file_to_buffer(filename,buffer);
            res.write(generateHttpResponse(buffer.size(),buffer));
        });

        get("/m3u8parse.html",[](const Request&req,Response&res){
            std::string filename="../source//m3u8parse.html";
            std::vector<char>&buffer=req.buffer;
            read_file_to_buffer(filename,buffer);
            res.write(generateHttpResponse(buffer.size(),buffer));
        });

        get("/hlsparse.html",[](const Request&req,Response&res){
            std::string filename="../source//hlsparse.html";
            std::vector<char>&buffer=req.buffer;
            read_file_to_buffer(filename,buffer);
            res.write(generateHttpResponse(buffer.size(),buffer));
        });

        get("/video",[](const Request&req,Response&res){
            std::string path="../source/video";
            std::string key_file_name="info.json";
            std::vector<char>&buffer=req.buffer;
            read_key_file_to_buffer(path,key_file_name,buffer);
            res.write(generateHttpResponse(buffer.size(),buffer,ContentType::JSON));
            auto temp=std::string(buffer.data(),buffer.data()+buffer.size());
            LOG(INFO)<<temp;
        });

        get("/video/helloworld_sp_01/newworld_sp_01.m3u8",[](const Request&req,Response&res){
            std::string filename="../source/video/helloworld_sp_01/newworld_sp_01.m3u8";
            std::vector<char>&buffer=req.buffer;
            read_file_to_buffer(filename,buffer);
            res.write(generateHttpResponse(buffer.size(),buffer,ContentType::M3U8));
        });

        get("/video/helloworld_sp_01/segment_",[](const Request&req,Response&res){
            std::string filename="../source/video/helloworld_sp_01/segment_"+req.queryParams.at("seg")+".ts";
            LOG(INFO)<<filename;
            std::vector<char>&buffer=req.buffer;
            read_file_to_buffer(filename,buffer);
            res.write(generateHttpResponse(buffer.size(),buffer,ContentType::TS));
        });

        get("/video/enc/test/output",[](const Request&req,Response&res){
            std::string filename="../source/video/enc/test/output"+req.queryParams.at("seg")+".ts";
            LOG(INFO)<<filename;
            std::vector<char>&buffer=req.buffer;
            read_file_to_buffer(filename,buffer);
            res.write(generateHttpResponse(buffer.size(),buffer,ContentType::TS));
        });

        post("/login",[](const Request&req,Response&res){
            LOG(INFO)<<"now post login";
            auto &buffer=req.buffer;
            LoginInfo info;
            info.fromJsonString(req.body);
            LOG(INFO)<<req.body;
            auto guard=ConnectionGuard(MysqlConnPoll::getInstance().getConnection(),MysqlConnPoll::getInstance());
            std::string query="select * from user where username='"+
            info.getUsername()+"' and pwd='"+info.getPassword()+"';";
            LOG(INFO)<<query;
            auto result=guard->query(query);
            if(result){
                LOG(INFO)<<"find user ";
                while(result->next()){
                    LOG(INFO)<<result->getString("username");
                    LOG(INFO)<<result->getString("pwd");
                    LOG(INFO)<<result->getInt("id");
                }
                info.status_code=200;
                info.token="temp_token";
                info.username="admin";
                info.role="管理员";
                info.level="VIP";
                info.lastLogin="刚刚";
                info.encryptAccess=true;
                //     userinfo: {
                //         username: "admin",
                //         role: "管理员",
                //         level: "VIP会员",
                //         lastLogin: "刚刚",
                //         encryptAccess: true
                //     }
                // };
            }else{
                LOG(ERROR)<<"not find";
            }
            //LOG(INFO)<<info.toJsonString();
            auto temp=info.toJsonString();
            LOG(INFO)<<temp;
            buffer.assign(temp.begin(),temp.end());
            res.write(generateHttpResponse(buffer.size(),buffer,ContentType::JSON));
        });

        get("/encrypt.html",[](const Request&req,Response&res){
            std::string filename="../source//encrypt.html";
            std::vector<char>&buffer=req.buffer;
            read_file_to_buffer(filename,buffer);
            res.write(generateHttpResponse(buffer.size(),buffer));
        });

        get("/video/enc/test/output.m3u8",[](const Request&req,Response&res){
            std::string filename="../source/video/enc/test/output.m3u8";
            std::vector<char>&buffer=req.buffer;
            read_file_to_buffer(filename,buffer);
            res.write(generateHttpResponse(buffer.size(),buffer,ContentType::M3U8));
        });

        get("/source/key/enc.key",[](const Request&req,Response&res){
            give_buffer_something("../source/key/enc.key",req.buffer,ContentType::KEY);
            //auto temp=std::string(req.buffer.data(),req.buffer.data()+req.buffer.size());
            //LOG(INFO)<<temp;
        });
    }

    static void give_buffer_something(const std::string &filename,
        std::vector<char>&buffer,ContentType cont=ContentType::HTML){
        read_file_to_buffer(filename,buffer);
        generateHttpResponse(buffer.size(),buffer,cont);
        return ;
    }
};
//int Router::grab_function_count=0;