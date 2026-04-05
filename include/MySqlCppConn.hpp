#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>
#include "Glog.hpp"

class MysqlConn {
public:
    // 构造函数和析构函数
    MysqlConn();
    ~MysqlConn();

    // 连接管理
    bool connect(const std::string& host, const std::string& user, 
                 const std::string& password, const std::string& database,
                 int port = 3306);
    void disconnect();
    bool isConnected() const;
    bool reconnect();

    // 基本查询操作
    bool execute(const std::string& sql);
    sql::ResultSet* query(const std::string& sql);
    
    // 预处理语句操作
    bool executePrepared(const std::string& sql, const std::vector<std::string>& params);
    sql::ResultSet* queryPrepared(const std::string& sql, const std::vector<std::string>& params);
    
    // 事务操作
    bool beginTransaction();
    bool commit();
    bool rollback();

    // 获取数据
    int getInt(const std::string& sql, const std::string& columnName);
    std::string getString(const std::string& sql, const std::string& columnName);
    bool getBool(const std::string& sql, const std::string& columnName);
    double getDouble(const std::string& sql, const std::string& columnName);

    // 批量操作
    bool insertBatch(const std::string& table, const std::vector<std::map<std::string, std::string>>& data);
    bool updateBatch(const std::string& table, const std::string& whereField, 
                     const std::map<std::string, std::vector<std::string>>& updates);

    // 表操作
    bool createTable(const std::string& tableName, const std::string& schema);
    bool dropTable(const std::string& tableName);
    bool tableExists(const std::string& tableName);

    // 错误信息
    std::string getLastError() const;
    int getLastErrorCode() const;

    // 连接信息
    std::string getConnectionInfo() const;

private:
    sql::mysql::MySQL_Driver* driver_;
    sql::Connection* connection_;
    std::string lastError_;
    int lastErrorCode_;
    bool isConnected_;

    // 禁止拷贝
    MysqlConn(const MysqlConn&) = delete;
    MysqlConn& operator=(const MysqlConn&) = delete;

    // 内部方法
    void setError(const sql::SQLException& e);
    void cleanupResultSet(sql::ResultSet* res);
    void cleanupStatement(sql::Statement* stmt);
    void cleanupPreparedStatement(sql::PreparedStatement* pstmt);
};