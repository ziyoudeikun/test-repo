#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <jsoncpp/json/json.h>
#include "Glog.hpp"

class JsonSerializer {
public:
    // 将对象序列化为 JSON 字符串
    virtual std::string toJsonString() const = 0;
    
    // 从 JSON 字符串反序列化对象
    virtual void fromJsonString(const std::string& jsonStr) = 0;
    
    // 将对象转换为 Json::Value
    virtual Json::Value toJsonValue() const = 0;
    
    // 从 Json::Value 恢复对象
    virtual void fromJsonValue(const Json::Value& jsonValue) = 0;
    
    virtual ~JsonSerializer() = default;
    
protected:
    // 工具函数：检查 JSON 解析是否成功
    bool parseJsonString(const std::string& jsonStr, Json::Value& result) const {
        Json::CharReaderBuilder readerBuilder;
        std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
        std::string errors;
        
        return reader->parse(jsonStr.c_str(), 
                           jsonStr.c_str() + jsonStr.length(), 
                           &result, 
                           &errors);
    }
    
    // 工具函数：将 Json::Value 转换为字符串
    std::string jsonValueToString(const Json::Value& jsonValue) const {
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "    ";
        return Json::writeString(writerBuilder, jsonValue);
    }
};

// 用户登录信息类
class LoginInfo : public JsonSerializer {
private:
public:
    int status_code;
    std::string token;
    std::string username;
    std::string role;
    std::string level;
    std::string lastLogin;
    bool encryptAccess;
    std::string password;
    std::string timestamp;
    std::string nonce;
    std::string app_id;
    std::string sign;
    //std::string username;
public:
    LoginInfo() = default;

    std::string toJsonString() const override {
        return jsonValueToString(toJsonValue());
    }
    
    void fromJsonString(const std::string& jsonStr) override {
        Json::Value root;
        if (parseJsonString(jsonStr, root)) {
            fromJsonValue(root);
        } else {
            throw std::runtime_error("JSON 解析失败");
        }
    }
    
    Json::Value toJsonValue() const override {
        Json::Value root;
        //root["success"]=success;
        // if(success){
        //     root["token"]=token;
        //     //["user"]["id"]=id;
        //     //root["user"]["username"]=name;
        // }else{
        //     root["message"]="用户名或密码错误";
        // }
        //status_code: 200,
        //     token: "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VybmFtZSI6ImFkbWluIiwicm9sZSI6IkFkbWluaXN0cmF0b3IiLCJsZXZlbCI6IlZJUCBtZW1iZXIiLCJsYXN0TG9naW4iOiJqdXN0IG5vdyIsImVuY3J5cHRBY2Nlc3MiOnRydWUsImFwcF9pZCI6Imhsc190b29sYm94X3dlYiIsImV4cCI6MTY4MDk0NzIwMH0.mocktoken12345",
        //     userinfo: {
        //         username: "admin",
        //         role: "管理员",
        //         level: "VIP会员",
        //         lastLogin: "刚刚",
        //         encryptAccess: true
        //     }
        root["status_code"]=status_code;
        root["token"]=token;
        auto& temp=root["userinfo"];
        temp["username"]=username;
        temp["role"]=role;
        temp["level"]=level;
        temp["lastLogin"]=lastLogin;
        temp["encryptAccess"]=true;
        return root;
    }
    
    void fromJsonValue(const Json::Value& jsonValue) override {
        if (jsonValue.isMember("username")) {
            username = jsonValue["username"].asString();
        }
        if (jsonValue.isMember("password")) {
            password = jsonValue["password"].asString();
        }
        if (jsonValue.isMember("timestamp")) {
            timestamp = jsonValue["timestamp"].asString();
        }
        if (jsonValue.isMember("nonce")) {
            nonce = jsonValue["nonce"].asString();
        }
        if (jsonValue.isMember("app_id")) {
            app_id = jsonValue["app_id"].asString();
        }
        if (jsonValue.isMember("sign")) {
            sign = jsonValue["sign"].asString();
        }
    }
    
    // Getter 和 Setter
    //void setUsername(const std::string& user) { username = user; }
    //void setPassword(const std::string& pwd) { password = pwd; }
    //std::string getLoginname() const { return loginname; }
    std::string getUsername() const { return username; }
    std::string getPassword() const { return password; }
    
    // void printInfo() const {
    //     LOG(INFO) << "用户名: " << username << std::endl;
    //     LOG(INFO) << "密码: " << password << std::endl;
    // }
};

class RequestJson : public JsonSerializer {
public:
    int status_code;
    bool success;
    std::string token;
    std::string user;
    int id;
    std::string name;
    std::string message;
public:
    RequestJson() = default;
    std::string toJsonString() const override {
        return jsonValueToString(toJsonValue());
    }
    
    void fromJsonString(const std::string& jsonStr) override {
        Json::Value root;
        if (parseJsonString(jsonStr, root)) {
            fromJsonValue(root);
        } else {
            throw std::runtime_error("JSON 解析失败");
        }
    }
    
    Json::Value toJsonValue() const override {
        Json::Value root;
        root["success"]=success;
        if(success){
            root["token"] = token;
            root["user"]["id"] = id;
            root["user"]["name"]=name;
        }else{
            root["message"]="用户名或密码错误";
        }
        return root;
    }
    
    void fromJsonValue(const Json::Value& jsonValue) override {
        // if (jsonValue.isMember("username")) {
        //     username = jsonValue["username"].asString();
        // }
        // if (jsonValue.isMember("password")) {
        //     password = jsonValue["password"].asString();
        // }
    }
    
    // Getter 和 Setter
    // void setUsername(const std::string& user) { username = user; }
    // void setPassword(const std::string& pwd) { password = pwd; }
    // std::string getUsername() const { return username; }
    // std::string getPassword() const { return password; }
    
    void printInfo() const {
        // LOG(INFO) << "用户名: " << username << std::endl;
        // LOG(INFO) << "密码: " << password << std::endl;
        LOG(INFO)<<"now json string:"<<toJsonString();
    }
};

class VarifyTokenJson : public JsonSerializer {
public:
    bool success;
    bool valid;
    std::string token;
    int id;
    std::string name;
    std::string username;
    std::string message;
public:
    VarifyTokenJson() = default;
    std::string toJsonString() const override {
        return jsonValueToString(toJsonValue());
    }
    
    void fromJsonString(const std::string& jsonStr) override {
        Json::Value root;
        if (parseJsonString(jsonStr, root)) {
            fromJsonValue(root);
        } else {
            throw std::runtime_error("JSON 解析失败");
        }
    }
    
    Json::Value toJsonValue() const override {
        Json::Value root;
        root["success"]=success;
        root["valid"]=valid;
        if(success){
            root["user"]["id"] = id;
            root["user"]["name"]=name;
            root["user"]["username"]=username;
        }
        root["message"]=message;
        if(!success){
            root["error_code"]="TOKEN_INVALID";
        }
        return root;
    }
    
    void fromJsonValue(const Json::Value& jsonValue) override {
        if (jsonValue.isMember("token")) {
            token = jsonValue["token"].asString();
        }
        // if (jsonValue.isMember("password")) {
        //     password = jsonValue["password"].asString();
        // }
    }
    
    // Getter 和 Setter
    // void setUsername(const std::string& user) { username = user; }
    // void setPassword(const std::string& pwd) { password = pwd; }
    // std::string getUsername() const { return username; }
    // std::string getPassword() const { return password; }
    
    void printInfo() const {
        // LOG(INFO) << "用户名: " << username << std::endl;
        // LOG(INFO) << "密码: " << password << std::endl;
        LOG(INFO)<<"now token string:"<<token;
    }
};
class GrabRedPacketJson : public JsonSerializer {
public:
    std::string token;
    std::string redpacketid;
public:
    GrabRedPacketJson() = default;
    std::string toJsonString() const override {
        return jsonValueToString(toJsonValue());
    }
    
    void fromJsonString(const std::string& jsonStr) override {
        Json::Value root;
        if (parseJsonString(jsonStr, root)) {
            fromJsonValue(root);
        } else {
            throw std::runtime_error("JSON 解析失败");
        }
    }
    
    Json::Value toJsonValue() const override {
        Json::Value root;
        root["token"]=token;
        root["redpacketid"]=redpacketid;
        
        return root;
    }
    
    void fromJsonValue(const Json::Value& jsonValue) override {
        // if (jsonValue.isMember("token")) {
        //     token = jsonValue["token"].asString();
        // }
        if (jsonValue.isMember("redPacketId")) {
            redpacketid = jsonValue["redPacketId"].asString();
        }
    }
    
    // Getter 和 Setter
    // void setUsername(const std::string& user) { username = user; }
    // void setPassword(const std::string& pwd) { password = pwd; }
    // std::string getUsername() const { return username; }
    // std::string getPassword() const { return password; }
    
    void printInfo() const {
        // LOG(INFO) << "用户名: " << username << std::endl;
        // LOG(INFO) << "密码: " << password << std::endl;
        LOG(INFO)<<"now token string:"<<token;
        LOG(INFO)<<"now red string:"<<redpacketid;
    }
};
/*
[
  {
    "id": 1,
    "name": "自然风光纪录片",
    "description": "高清自然风光纪录片，展示世界各地的壮丽景色。",
    "cover": "https://images.unsplash.com/photo-1506905925346-21bda4d32df4",
    "url": "https://test-streams.mux.dev/x36xhzz/x36xhzz.m3u8",
    "size": "245.8 MB",
    "duration": "15:30",
    "updated": "2023-11-05 14:30:22",
    "bitrate": "3.5 Mbps",
    "resolution": "1920x1080",
    "framerate": "30 fps",
    "codec": "H.264/AAC",
    "category": "纪录片",
    "tags": ["自然", "风光", "高清"]
  },
  {
    "id": 2,
    "name": "城市延时摄影",
    "description": "全球各大城市的延时摄影集锦，展现都市的繁华与节奏。",
    "cover": "https://images.unsplash.com/photo-1477959858617-67f85cf4f1df",
    "url": "https://content.jwplatform.com/manifests/vM7nH0Kl.m3u8",
    "size": "128.4 MB",
    "duration": "08:45",
    "updated": "2023-11-04 09:15:33",
    "bitrate": "2.1 Mbps",
    "resolution": "1280x720",
    "framerate": "24 fps",
    "codec": "H.264/AAC",
    "category": "艺术",
    "tags": ["城市", "延时", "摄影"]
  }
]
*/
class InfoJson : public JsonSerializer {
public:
    int id;
    std::string name;
    std::string description;
    std::string cover;
    std::string url;
    std::string size;
    std::string duration;
    std::string updated;
    std::string bitrate;
    std::string resolution;
    std::string framerate;
    std::string codec;
    std::string category;
    std::string tags;
    std::vector<std::string>jsons;
public:
    InfoJson() = default;
    InfoJson(const std::string&id){

    }
    std::string toJsonString() const override {
        return jsonValueToString(toJsonValue());
    }
    
    void fromJsonString(const std::string& jsonStr) override {
        Json::Value root;
        if (parseJsonString(jsonStr, root)) {
            fromJsonValue(root);
        } else {
            throw std::runtime_error("JSON 解析失败");
        }
    }
    
    Json::Value toJsonValue() const override {
        Json::Value root;
        //root.append
        // root["success"]=success;
        // if(success){
        //     root["amount"]=amount;
        //     root["message"]="抢红包成功";
        //     root["currency"]="CNY";
        //     root["redPacketId"]=redpacketid;
        // }else{
        //     root["message"]=message;
        // }
        
        return root;
    }
    
    void fromJsonValue(const Json::Value& jsonValue) override {

        // if (jsonValue.isMember("token")) {
        //     token = jsonValue["token"].asString();
        // }
        // if (jsonValue.isMember("amount")) {
        //     amount = jsonValue["amount"].asDouble();
        // }
        // if (jsonValue.isMember("success")) {
        //     success = jsonValue["success"].asBool();
        // }
        // if(jsonValue.isMember("message")){
        //     message=jsonValue["message"].asString();
        // }
    }
    
    // Getter 和 Setter
    // void setUsername(const std::string& user) { username = user; }
    // void setPassword(const std::string& pwd) { password = pwd; }
    // std::string getUsername() const { return username; }
    // std::string getPassword() const { return password; }

    bool setJsons(const std::string&json){
        jsons.emplace_back(json);
        return true;
    }
    
    void printInfo() const {
        // LOG(INFO) << "用户名: " << username << std::endl;
        // LOG(INFO) << "密码: " << password << std::endl;
        //LOG(INFO)<<"now token string:"<<token;
        //LOG(INFO)<<"now red string:"<<redpacketid;
    }
};