#include "../include/MySqlCppConn.hpp"

MysqlConn::MysqlConn() 
    : driver_(nullptr), connection_(nullptr), lastErrorCode_(0), isConnected_(false) {
    try {
        driver_ = sql::mysql::get_mysql_driver_instance();
    } catch (const sql::SQLException& e) {
        setError(e);
    }
}

MysqlConn::~MysqlConn() {
    //LOG(INFO)<<"conn is ~()";
    disconnect();
}

bool MysqlConn::connect(const std::string& host, const std::string& user, 
                       const std::string& password, const std::string& database,
                       int port) {
    try {
        std::string connectionStr = "tcp://" + host + ":" + std::to_string(port);
        connection_ = driver_->connect(connectionStr, user, password);
        connection_->setSchema(database);
        isConnected_ = true;
        lastError_.clear();
        lastErrorCode_ = 0;
        return true;
    } catch (const sql::SQLException& e) {
        setError(e);
        isConnected_ = false;
        return false;
    }
}

void MysqlConn::disconnect() {
    if (connection_) {
        try {
            delete connection_;
            connection_ = nullptr;
        } catch (...) {
            // 忽略析构异常
        }
    }
    isConnected_ = false;
}

bool MysqlConn::isConnected() const {
    return isConnected_ && connection_ && !connection_->isClosed();
}

bool MysqlConn::reconnect() {
    if (connection_) {
        try {
            return connection_->reconnect();
        } catch (const sql::SQLException& e) {
            setError(e);
            return false;
        }
    }
    return false;
}

bool MysqlConn::execute(const std::string& sql) {
    if (!isConnected()) return false;

    sql::Statement* stmt = nullptr;
    try {
        stmt = connection_->createStatement();
        bool result = stmt->execute(sql);
        cleanupStatement(stmt);
        return result;
    } catch (const sql::SQLException& e) {
        cleanupStatement(stmt);
        setError(e);
        return false;
    }
}

sql::ResultSet* MysqlConn::query(const std::string& sql) {
    if (!isConnected()) return nullptr;

    sql::Statement* stmt = nullptr;
    try {
        stmt = connection_->createStatement();
        sql::ResultSet* res = stmt->executeQuery(sql);
        // 注意：调用者需要负责释放 ResultSet
        return res;
    } catch (const sql::SQLException& e) {
        cleanupStatement(stmt);
        setError(e);
        return nullptr;
    }
}

bool MysqlConn::executePrepared(const std::string& sql, const std::vector<std::string>& params) {
    if (!isConnected()) return false;

    sql::PreparedStatement* pstmt = nullptr;
    try {
        pstmt = connection_->prepareStatement(sql);
        
        for (size_t i = 0; i < params.size(); ++i) {
            pstmt->setString(i + 1, params[i]);
        }
        
        bool result = pstmt->execute();
        cleanupPreparedStatement(pstmt);
        return result;
    } catch (const sql::SQLException& e) {
        cleanupPreparedStatement(pstmt);
        setError(e);
        return false;
    }
}

sql::ResultSet* MysqlConn::queryPrepared(const std::string& sql, const std::vector<std::string>& params) {
    if (!isConnected()) return nullptr;

    sql::PreparedStatement* pstmt = nullptr;
    try {
        pstmt = connection_->prepareStatement(sql);
        
        for (size_t i = 0; i < params.size(); ++i) {
            pstmt->setString(i + 1, params[i]);
        }
        
        sql::ResultSet* res = pstmt->executeQuery();
        // 注意：调用者需要负责释放 ResultSet
        return res;
    } catch (const sql::SQLException& e) {
        cleanupPreparedStatement(pstmt);
        setError(e);
        return nullptr;
    }
}

bool MysqlConn::beginTransaction() {
    if (!isConnected()) return false;
    
    try {
        connection_->setAutoCommit(false);
        return true;
    } catch (const sql::SQLException& e) {
        setError(e);
        return false;
    }
}

bool MysqlConn::commit() {
    if (!isConnected()) return false;
    
    try {
        connection_->commit();
        connection_->setAutoCommit(true);
        return true;
    } catch (const sql::SQLException& e) {
        setError(e);
        return false;
    }
}

bool MysqlConn::rollback() {
    if (!isConnected()) return false;
    
    try {
        connection_->rollback();
        connection_->setAutoCommit(true);
        return true;
    } catch (const sql::SQLException& e) {
        setError(e);
        return false;
    }
}

int MysqlConn::getInt(const std::string& sql, const std::string& columnName) {
    sql::ResultSet* res = query(sql);
    if (!res) return 0;

    int value = 0;
    try {
        if (res->next()) {
            value = res->getInt(columnName);
        }
        cleanupResultSet(res);
    } catch (...) {
        cleanupResultSet(res);
    }
    return value;
}

std::string MysqlConn::getString(const std::string& sql, const std::string& columnName) {
    sql::ResultSet* res = query(sql);
    if (!res) return "";

    std::string value;
    try {
        if (res->next()) {
            value = res->getString(columnName);
        }
        cleanupResultSet(res);
    } catch (...) {
        cleanupResultSet(res);
    }
    return value;
}

bool MysqlConn::insertBatch(const std::string& table, const std::vector<std::map<std::string, std::string>>& data) {
    if (data.empty()) return true;

    beginTransaction();
    
    try {
        for (const auto& row : data) {
            std::string columns, values;
            for (const auto& field : row) {
                if (!columns.empty()) columns += ", ";
                if (!values.empty()) values += ", ";
                columns += field.first;
                values += "'" + field.second + "'";
            }
            
            std::string sql = "INSERT INTO " + table + " (" + columns + ") VALUES (" + values + ")";
            if (!execute(sql)) {
                rollback();
                return false;
            }
        }
        
        return commit();
    } catch (...) {
        rollback();
        return false;
    }
}

bool MysqlConn::createTable(const std::string& tableName, const std::string& schema) {
    std::string sql = "CREATE TABLE IF NOT EXISTS " + tableName + " " + schema;
    return execute(sql);
}

bool MysqlConn::tableExists(const std::string& tableName) {
    std::string sql = "SHOW TABLES LIKE '" + tableName + "'";
    sql::ResultSet* res = query(sql);
    if (!res) return false;

    bool exists = res->next();
    cleanupResultSet(res);
    return exists;
}

std::string MysqlConn::getLastError() const {
    return lastError_;
}

int MysqlConn::getLastErrorCode() const {
    return lastErrorCode_;
}

std::string MysqlConn::getConnectionInfo() const {
    if (!isConnected()) return "Not connected";
    return "Connected to database";
}

// 私有方法实现
void MysqlConn::setError(const sql::SQLException& e) {
    lastError_ = e.what();
    lastErrorCode_ = e.getErrorCode();
}

void MysqlConn::cleanupResultSet(sql::ResultSet* res) {
    if (res) {
        try {
            delete res;
        } catch (...) {
            // 忽略析构异常
        }
    }
}

void MysqlConn::cleanupStatement(sql::Statement* stmt) {
    if (stmt) {
        try {
            delete stmt;
        } catch (...) {
            // 忽略析构异常
        }
    }
}

void MysqlConn::cleanupPreparedStatement(sql::PreparedStatement* pstmt) {
    if (pstmt) {
        try {
            delete pstmt;
        } catch (...) {
            // 忽略析构异常
        }
    }
}