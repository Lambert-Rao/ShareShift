//
// Created by lambert on 23-3-26.
//

#pragma once

#include <list>
#include <mysql/mysql.h>
#include <string>
#include <map>
#include <condition_variable>

#include "ss_logging.h"

constexpr char reconnect = true;
constexpr int min_conn_cnt = 5;


class MySqlPool;

class ResultSet
{
public:
    explicit ResultSet(MYSQL_RES *res);

    ~ResultSet();

    bool Next();

    int GetInt(const char *key);

    char *GetString(const char *key);

private:
    int GetIndex(const char *key);

    MYSQL_RES *res_;//the result of the query
    MYSQL_ROW row_{};
    std::map<std::string, size_t> key_map_;//<key, index>,e.g. <"id", 0>, <"name", 1>, <"age", 2>
};

//prepare statement for insert, update
class PrepareStatement
{
public:
    PrepareStatement() = default;

    ~PrepareStatement();

    bool Initialize(MYSQL *mysql, const std::string &sql);

    template<typename T>
    void SetParameter(uint32_t index, T &value);

    bool ExecuteUpdate();

    uint32_t GetInsertId();

private:
    MYSQL_STMT *statement_{};//the statement
    MYSQL_BIND *parameter_bind_{};//the bind of the paramerter
    uint32_t parameter_count_{};
};

template<typename T>
void PrepareStatement::SetParameter(uint32_t index, T &value)
{
    if (index >= parameter_count_)
    {
        LOG_ERROR << "index out of range";
        return;
    }
    if (std::is_same<T, int>::value || std::is_same<T, int32_t>::value)
    {
        parameter_bind_[index].buffer_type = MYSQL_TYPE_LONG;
        parameter_bind_[index].buffer = &value;
    } else if (std::is_same<T, const std::string>::value || std::is_same<T, std::string>::value)
    {
        parameter_bind_[index].buffer_type = MYSQL_TYPE_STRING;
        parameter_bind_[index].buffer = &value;
        parameter_bind_[index].buffer_length = value.length();
    } else
    {
        LOG_ERROR << "unsupported type";
    }
}


//class to do the database query, one connection one object
class MySqlConnection
{
public:
    explicit MySqlConnection(MySqlPool *pMySQLPool);

    ~MySqlConnection();

    //use return number to represent the status of the connection
    int Initialize();

    //database query interface
    //use return boolean to represent success or failure
    bool ExecuteCreate(const char *sql_query);

    bool ExecuteDrop(const char *sql_query);

    //for a query which returns a result set like select
    ResultSet *ExecuteQuery(const char *sql_query);

    //for a query which does not return a result set
    bool ExecutePassQuery(const char *sql_query);

    bool ExecuteUpdate(const char *sql_query, bool care_affected_rows = true);

    //transaction interface
    bool StartTransaction();

    bool Commit();

    bool Rollback();

    ::uint32_t GetInsertId();

    const char *GetPoolName();

private:
    int row_num_{0};
    MySqlPool *pMySQLPool_{};//the pool which this connection belongs to
    MYSQL *mysql_{};//the connection
};

//class to manage MySql connections
class MySqlPool
{
public:
    MySqlPool() = delete;

    MySqlPool(const char *pool_name, const char *ip,
              u_int16_t port, const char *user, const char *passwd,
              const char *db_name,
              u_int16_t max_conn_cnt);

    ~MySqlPool();

    int Initialize();//connect to the database and create connections
    MySqlConnection *GetConnection(const int time_out_ms);//get a connection from the pool
    void ReleaseConnection(MySqlConnection *pConn);//release a connection to the pool

    const char *GetPoolName() const
    { return pool_name_; }

    const char *GetIp() const
    { return ip_; }

    u_int16_t GetPort() const
    { return port_; }

    const char *GetUser() const
    { return user_; }

    const char *GetPassword() const
    { return passwd_; }

    const char *GetDbName() const
    { return db_name_; }

private:
    const char *pool_name_;
    const char *ip_;
    u_int16_t port_;
    const char *user_;
    const char *passwd_;
    const char *db_name_;
    u_int16_t max_conn_cnt_;
    std::list<MySqlConnection *> free_conn_list_;
    std::list<MySqlConnection *> used_conn_list_;
    std::mutex mutex_;
    std::condition_variable cond_var_;
    bool request_abort_{false};
    int current_conn_count_{0};
    int max_conn_count_{0};
};

//manage all the pools
//manager can read config file and create multiple mysql servers, including master and slaves
//singleton
class MySqlManager
{
public:
    MySqlManager() = default;

    ~MySqlManager() = default;

    static MySqlManager *GetInstance();

    int Initialize();

    auto GetConnection(const char *pool_name) -> MySqlConnection *;

    void ReleaseConnection(MySqlConnection *pConn);

private:
    static MySqlManager *static_mysql_manager_;
    std::map<std::string, MySqlPool *> pool_map_;
};

//auto release connection
class ConnectionGuard
{
public:
    ConnectionGuard(MySqlConnection *pconnection) : connection_(pconnection)
    {}

    ~ConnectionGuard()
    {
        if (connection_)
        {
            MySqlManager::GetInstance()->ReleaseConnection(connection_);
        }
    }

private:
    MySqlConnection *connection_ = nullptr;
};