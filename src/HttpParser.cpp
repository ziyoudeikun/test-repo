#include "../include/HttpParser.hpp"
#include <iostream>
#include <cstring>

HttpParser::HttpParser(http_parser_type type) {
    // 初始化 http-parser
    http_parser_init(&parser_, type);
    parser_.data = this;  // 设置用户数据指针

    // 初始化设置
    memset(&settings_, 0, sizeof(settings_));
    settings_.on_url = onUrl;
    settings_.on_header_field = onHeaderField;
    settings_.on_header_value = onHeaderValue;
    settings_.on_headers_complete = onHeadersComplete;
    settings_.on_body = onBody;
    settings_.on_message_complete = onMessageComplete;
}

HttpParser::~HttpParser() {
    // 清理工作
}

// 静态回调函数
int HttpParser::onUrl(http_parser* parser, const char* at, size_t length) {
    HttpParser* self = static_cast<HttpParser*>(parser->data);
    return self->handleUrl(at, length);
}

int HttpParser::onHeaderField(http_parser* parser, const char* at, size_t length) {
    HttpParser* self = static_cast<HttpParser*>(parser->data);
    return self->handleHeaderField(at, length);
}

int HttpParser::onHeaderValue(http_parser* parser, const char* at, size_t length) {
    HttpParser* self = static_cast<HttpParser*>(parser->data);
    return self->handleHeaderValue(at, length);
}

int HttpParser::onHeadersComplete(http_parser* parser) {
    HttpParser* self = static_cast<HttpParser*>(parser->data);
    return self->handleHeadersComplete();
}

int HttpParser::onBody(http_parser* parser, const char* at, size_t length) {
    HttpParser* self = static_cast<HttpParser*>(parser->data);
    return self->handleBody(at, length);
}

int HttpParser::onMessageComplete(http_parser* parser) {
    HttpParser* self = static_cast<HttpParser*>(parser->data);
    return self->handleMessageComplete();
}

// 实例方法实现
int HttpParser::handleUrl(const char* at, size_t length) {
    url_.assign(at, length);
    if (url_callback_) {
        url_callback_(url_);
    }
    return 0;
}

int HttpParser::handleHeaderField(const char* at, size_t length) {
    // 如果之前有完整的header field-value对，先保存
    if (!current_field_.empty() && !current_value_.empty()) {
        headers_[current_field_] = current_value_;
        if (header_callback_) {
            header_callback_(current_field_, current_value_);
        }
        current_field_.clear();
        current_value_.clear();
    }
    
    current_field_.append(at, length);
    return 0;
}

int HttpParser::handleHeaderValue(const char* at, size_t length) {
    current_value_.append(at, length);
    return 0;
}

int HttpParser::handleHeadersComplete() {
    // 保存最后一个header
    if (!current_field_.empty() && !current_value_.empty()) {
        headers_[current_field_] = current_value_;
        if (header_callback_) {
            header_callback_(current_field_, current_value_);
        }
        current_field_.clear();
        current_value_.clear();
    }
    
    headers_complete_ = true;
    if (headers_complete_callback_) {
        headers_complete_callback_();
    }
    return 0;
}

int HttpParser::handleBody(const char* at, size_t length) {
    body_.append(at, length);
    if (body_callback_) {
        body_callback_(body_);
    }
    return 0;
}

int HttpParser::handleMessageComplete() {
    message_complete_ = true;
    if (message_complete_callback_) {
        message_complete_callback_();
    }
    return 0;
}

// 设置回调函数
void HttpParser::setUrlCallback(UrlCallback callback) {
    url_callback_ = std::move(callback);
}

void HttpParser::setHeaderCallback(HeaderCallback callback) {
    header_callback_ = std::move(callback);
}

void HttpParser::setBodyCallback(BodyCallback callback) {
    body_callback_ = std::move(callback);
}

void HttpParser::setMessageCompleteCallback(MessageCompleteCallback callback) {
    message_complete_callback_ = std::move(callback);
}

void HttpParser::setHeadersCompleteCallback(HeadersCompleteCallback callback) {
    headers_complete_callback_ = std::move(callback);
}

// 解析数据
size_t HttpParser::parse(const char* data, size_t len) {
    return http_parser_execute(&parser_, &settings_, data, len);
}

size_t HttpParser::parse(const std::string& data) {
    return parse(data.data(), data.length());
}

// 获取解析结果
std::string HttpParser::getUrl() const {
    return url_;
}

std::string HttpParser::getBody() const {
    return body_;
}

std::string HttpParser::getMethod() const {
    if (parser_.type == HTTP_REQUEST) {
        return http_method_str(static_cast<http_method>(parser_.method));
    }
    return "";
}

int HttpParser::getStatusCode() const {
    if (parser_.type == HTTP_RESPONSE) {
        return parser_.status_code;
    }
    return 0;
}

const std::unordered_map<std::string, std::string>& HttpParser::getHeaders() const {
    return headers_;
}

bool HttpParser::isComplete() const {
    return message_complete_;
}

bool HttpParser::shouldKeepAlive() const {
    return http_should_keep_alive(&parser_) != 0;
}

// 重置解析器
void HttpParser::reset(http_parser_type type) {
    http_parser_init(&parser_, type);
    parser_.data = this;
    
    url_.clear();
    body_.clear();
    headers_.clear();
    current_field_.clear();
    current_value_.clear();
    message_complete_ = false;
    headers_complete_ = false;
}