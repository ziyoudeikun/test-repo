#pragma once
#include <http_parser.h>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>

class HttpParser {
public:
    // 回调函数类型定义
    using UrlCallback = std::function<void(const std::string&)>;
    using HeaderCallback = std::function<void(const std::string&, const std::string&)>;
    using BodyCallback = std::function<void(const std::string&)>;
    using MessageCompleteCallback = std::function<void()>;
    using HeadersCompleteCallback = std::function<void()>;

    // 构造函数
    explicit HttpParser(http_parser_type type = HTTP_REQUEST);
    ~HttpParser();

    // 删除拷贝构造和赋值
    HttpParser(const HttpParser&) = delete;
    HttpParser& operator=(const HttpParser&) = delete;

    // 设置回调函数
    void setUrlCallback(UrlCallback callback);
    void setHeaderCallback(HeaderCallback callback);
    void setBodyCallback(BodyCallback callback);
    void setMessageCompleteCallback(MessageCompleteCallback callback);
    void setHeadersCompleteCallback(HeadersCompleteCallback callback);

    // 解析数据
    size_t parse(const char* data, size_t len);
    size_t parse(const std::string& data);

    // 获取解析结果
    std::string getUrl() const;
    std::string getBody() const;
    std::string getMethod() const;
    int getStatusCode() const;
    const std::unordered_map<std::string, std::string>& getHeaders() const;
    bool isComplete() const;
    bool shouldKeepAlive() const;

    // 重置解析器
    void reset(http_parser_type type);

private:
    // 静态回调函数（C接口）
    static int onUrl(http_parser* parser, const char* at, size_t length);
    static int onHeaderField(http_parser* parser, const char* at, size_t length);
    static int onHeaderValue(http_parser* parser, const char* at, size_t length);
    static int onHeadersComplete(http_parser* parser);
    static int onBody(http_parser* parser, const char* at, size_t length);
    static int onMessageComplete(http_parser* parser);

    // 实例方法
    int handleUrl(const char* at, size_t length);
    int handleHeaderField(const char* at, size_t length);
    int handleHeaderValue(const char* at, size_t length);
    int handleHeadersComplete();
    int handleBody(const char* at, size_t length);
    int handleMessageComplete();

    // 成员变量
    http_parser parser_;
    http_parser_settings settings_;
    
    // 解析状态
    std::string current_field_;
    std::string current_value_;
    std::string url_;
    std::string body_;
    std::unordered_map<std::string, std::string> headers_;
    bool message_complete_ = false;
    bool headers_complete_ = false;

    // 回调函数
    UrlCallback url_callback_;
    HeaderCallback header_callback_;
    BodyCallback body_callback_;
    MessageCompleteCallback message_complete_callback_;
    HeadersCompleteCallback headers_complete_callback_;
};
