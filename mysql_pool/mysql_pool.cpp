//
// Created by lambert on 23-3-26.
//

#include <stdexcept>
#include <mutex>
#include <algorithm>

#include "mysql_pool.h"

/*--------------ResultSet--------------*/

ResultSet::ResultSet(MYSQL_RES *res)
        : res_(res)
{
    size_t field_num = mysql_num_fields(res_);//number of rows in the result set
    MYSQL_FIELD *fields = mysql_fetch_fields(res_);//get the fields of the result set
    for (size_t i = 0; i != field_num; i++)
    {
        key_map_[fields[i].name] = i;
    }

}

ResultSet::~ResultSet()
{
    if (res_)
        mysql_free_result(res_), res_ = nullptr;
}

bool ResultSet::Next()
{
    row_ = mysql_fetch_row(res_);
    if (row_)
        return true;
    else
        return false;
}

int ResultSet::GetIndex(const char *key)
{
    auto it = key_map_.find(key);
    if (it != key_map_.end())
        return it->second;
    else
        return -1;
}

int ResultSet::GetInt(const char *key)
{
    int index = GetIndex(key);
    if (index != -1)
    {
        return std::stoi(row_[index]);
    }
    else
        return -1;
}

char *ResultSet::GetString(const char *key)
{
    int index = GetIndex(key);
    if (index != -1)
    {
        return row_[index];
    }
    else
        return nullptr;
}

/*--------------PrepareStatement--------------*/

PrepareStatement::~PrepareStatement()
{
    if(statement_)
        mysql_stmt_close(statement_), statement_ = nullptr;
    if(parameter_bind_)
        delete[] parameter_bind_, parameter_bind_ = nullptr;
}

bool PrepareStatement::Initialize(MYSQL *mysql, const std::string &sql)
{
    mysql_ping(mysql);//when the connection is broken, ping to reconnect

    //statement for DML
    statement_ = mysql_stmt_init(mysql);
    if(!statement_)
    {
        LOG_ERROR << "mysql_stmt_init failed";
        return false;
    }

    if(mysql_stmt_prepare(statement_, sql.c_str(), sql.size()))
    {
        LOG_ERROR << "mysql_stmt_prepare failed";
        return false;
    }

    //count of '?' in the sql
    parameter_count_ = mysql_stmt_param_count(statement_);
    if(parameter_count_ > 0)
    {
        //we use statement in order to accelerate the query
        parameter_bind_ = new MYSQL_BIND[parameter_count_];
        if(!parameter_bind_)
        {
            LOG_ERROR << "new MYSQL_BIND failed";
            return false;
        }
        memset(parameter_bind_, 0, sizeof(MYSQL_BIND) * parameter_count_);
    }
    return true;
}

bool PrepareStatement::ExecuteUpdate()
{
    if(!statement_)
    {
        LOG_ERROR << "statement is null";
        return false;
    }
    if(mysql_stmt_bind_param(statement_, parameter_bind_))
    {
        LOG_ERROR << "mysql_stmt_bind_param failed"<< mysql_stmt_error(statement_);
        return false;
    }
    if(mysql_stmt_execute(statement_))
    {
        LOG_ERROR << "mysql_stmt_execute failed"<< mysql_stmt_error(statement_);
        return false;
    }
    if(mysql_stmt_affected_rows(statement_) == 0)
    {
        LOG_ERROR << "mysql_stmt affected none rows:"<< mysql_stmt_error(statement_);
        return false;
    }
    return true;
}

std::uint32_t PrepareStatement::GetInsertId()
{
    return mysql_stmt_insert_id(statement_);
}


/*--------------MySqlConnection--------------*/

MySqlConnection::MySqlConnection(MySqlPool *pMySQLPool)
{
    pMySQLPool_ = pMySQLPool;
    mysql_ = nullptr;//the connection
}

MySqlConnection::~MySqlConnection()
{
    if(mysql_)
        mysql_close(mysql_), mysql_ = nullptr;
}

//retrn value for state
int MySqlConnection::Initialize()
{
    mysql_ = mysql_init(nullptr);
    if(mysql_ == nullptr)
    {
        LOG_ERROR << "mysql_init failed";
        return 1;
    }

    //reconnect is a constexpr in mysql_pool.h
    mysql_options(mysql_, MYSQL_SET_CHARSET_NAME, "utf8mb4");
    mysql_options(mysql_, MYSQL_OPT_RECONNECT, &reconnect);

    if(!mysql_real_connect(mysql_, pMySQLPool_->GetIp(),pMySQLPool_->GetUser(), pMySQLPool_->GetPassword(),
                           pMySQLPool_->GetDbName(), pMySQLPool_->GetPort(), nullptr, 0))
    {
        LOG_ERROR << "mysql_real_connect failed";
        return 2;
    }

    return 0;
}

const char* MySqlConnection::GetPoolName()
{
    return pMySQLPool_->GetPoolName();
}

bool MySqlConnection::ExecuteCreate(const char *sql)
{
    if(mysql_query(mysql_, sql))
    {
        LOG_ERROR << "mysql_query failed"<< mysql_error(mysql_);
        return false;
    }
    return true;
}

bool MySqlConnection::ExecuteDrop(const char *sql)
{
    if(mysql_query(mysql_, sql))
    {
        LOG_ERROR << "mysql_query failed"<< mysql_error(mysql_);
        return false;
    }
    return true;
}

ResultSet* MySqlConnection::ExecuteQuery(const char *sql_query)
{
    mysql_ping(mysql_);//when the connection is broken, ping to reconnect
    row_num_ = 0;
    if(mysql_query(mysql_, sql_query))
    {
        LOG_ERROR << "mysql_query failed"<< mysql_error(mysql_);
        return nullptr;
    }

    MYSQL_RES *res = mysql_store_result(mysql_);
    if(!res)
    {
        LOG_ERROR << "mysql_store_result failed"<< mysql_error(mysql_);
        return nullptr;
    }
    row_num_ = static_cast<int>(mysql_num_rows(res));
    LOG_INFO << "row_num_:" << row_num_;
    auto *result_set = new ResultSet(res);
    return result_set;
}

bool MySqlConnection::ExecutePassQuery(const char *sql_query)
{
    mysql_ping(mysql_);
    if(mysql_query(mysql_, sql_query))
    {
        LOG_ERROR << "mysql_query failed"<< mysql_error(mysql_);
        return false;
    }
    return true;
}

bool MySqlConnection::ExecuteUpdate(const char *sql_query, bool care_affected_rows)
{
    mysql_ping(mysql_);
    if(mysql_query(mysql_, sql_query))
    {
        LOG_ERROR << "mysql_query failed"<< mysql_error(mysql_);
        return false;
    }
    if(mysql_affected_rows(mysql_)>0)
        return true;
    else
    {
        if(care_affected_rows)
        {
            LOG_ERROR << "mysql_affected_rows failed"<< mysql_error(mysql_);
            return false;
        }
        else
        {
            LOG_WARN << "mysql_affected_rows == 0"<< mysql_error(mysql_);
            return true;
        }
    }

}

bool MySqlConnection::StartTransaction()
{
    mysql_ping(mysql_);
    if(mysql_real_query(mysql_, "START TRANSACTION", 17) != 0)
    {
        LOG_ERROR << "mysql_real_query failed"<< mysql_error(mysql_);
        return false;
    }
    return true;
}

bool MySqlConnection::Commit()
{
    mysql_ping(mysql_);
    if(mysql_real_query(mysql_, "COMMIT", 6) != 0)
    {
        LOG_ERROR << "mysql_real_query failed"<< mysql_error(mysql_);
        return false;
    }
    return true;
}

bool MySqlConnection::Rollback()
{
    mysql_ping(mysql_);
    if(mysql_real_query(mysql_, "ROLLBACK", 8) != 0)
    {
        LOG_ERROR << "mysql_real_query failed"<< mysql_error(mysql_);
        return false;
    }
    return true;
}

auto MySqlConnection::GetInsertId() -> std::uint32_t
{
    return static_cast<std::uint32_t>(mysql_insert_id(mysql_));
}

/*--------------MySqlPool--------------*/

MySqlPool::~MySqlPool()//free all the connections from the pool
{
    std::lock_guard<std::mutex> lock(mutex_);
    request_abort_ = true;//set the abort flag to true
    cond_var_.notify_all();//notify all the thread to exit

    for(auto &connection : free_conn_list_)
    {
        delete connection;
    }
    free_conn_list_.clear();
}

MySqlPool::MySqlPool(const char *pool_name, const char *ip, u_int16_t port, const char *user, const char *passwd,
                     u_int16_t max_conn_cnt)
{
    pool_name_ = pool_name;
    ip_ = ip;
    port_ = port;
    user_ = user;
    passwd_ = passwd;
    max_conn_cnt_ = max_conn_cnt;
    current_conn_count_ = min_conn_cnt;
    request_abort_ = false;
}

int MySqlPool::Initialize()
{
    for(auto i = 0; i < current_conn_count_; ++i)
    {
        auto *connection = new MySqlConnection(this);
        if(connection->Initialize() != 0)
        {
            LOG_ERROR << "connection->Initialize() failed";
            return 1;
        }
        free_conn_list_.push_back(connection);
    }
    return 0;
}

MySqlConnection *MySqlPool::GetConnection(int timeout_ms)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if(request_abort_)
    {
        LOG_WARN << "going to exit";
        return nullptr;
    }

    //condition: when there is no free connection in the pool, wait for the condition to be true
    if(free_conn_list_.empty())
    {
        //wait for the condition to be true
        if(current_conn_count_ >= max_conn_cnt_)
        {
            //cotinue to wait for the condition to be true
            if(timeout_ms < 0)
            {
                cond_var_.wait(lock, [this] { return !free_conn_list_.empty() || request_abort_; });
            }
            else
            {
                cond_var_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                                   [this] { return !free_conn_list_.empty() || request_abort_; });
                if(free_conn_list_.empty())
                {
                    LOG_WARN << "timeout";
                    return nullptr;
                }
            }
            if(request_abort_)
            {
                LOG_WARN << "going to exit";
                return nullptr;
            }
        }
        else //create a new connection and add it to the pool
        {
            auto *connection = new MySqlConnection(this);
            if(connection->Initialize() != 0)
            {
                LOG_ERROR << "connection->Initialize() failed";
                return nullptr;
            }
            ++current_conn_count_;
            free_conn_list_.push_back(connection);
        }
    }

    //get a free connection directly or wait until a connection is available
    auto *connection = free_conn_list_.front();
    free_conn_list_.pop_front();
    return connection;
}

void MySqlPool::ReleaseConnection(MySqlConnection *connection)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find(free_conn_list_.begin(), free_conn_list_.end(), connection);
    if(it!=free_conn_list_.end())
    {
        LOG_WARN << "connection is already in the pool";
        return;
    }
    else
    {
        free_conn_list_.push_back(connection);
        cond_var_.notify_one();
    }

}

MySqlManager *MySqlManager::GetInstance()
{
    if(static_mysql_manager_ == nullptr)
    {
        static_mysql_manager_ = new MySqlManager();
        if(static_mysql_manager_->Initialize() != 0)
        {
            LOG_ERROR << "static_mysql_manager_->Initialize() failed";
            delete static_mysql_manager_;
            return nullptr;
        }
    }
    else
        return static_mysql_manager_;
}

int MySqlManager::Initialize()
{
    ConfigFileReader config_file("shareshift.conf");

}