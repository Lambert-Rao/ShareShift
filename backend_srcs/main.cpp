//
// Created by lambert on 23-3-26.
//

#include <csignal>

#include "mysql_pool.h"
#include "ss_logging.h"
#include "async_logging.h"
#include "cache_pool.h"
#include "ConfigFileReader.h"


// log settings
off_t kRollSize = 1 * 1000 * 1000; // only set 1M
static AsyncLogging *g_asyncLog = nullptr;

static void asyncOutput(const char *msg, int len) {
    g_asyncLog->append(msg, len);
}

int initLog()
{
    g_logLevel = Logger::INFO;   // set log level
    char name[256] = "ShareShift"; //set log name ShareShift.log
    // roll size kRollSize (1M), max 1 second flush to disk (flush)
    AsyncLogging *log =
        new AsyncLogging(::basename(name), kRollSize,
                         1);
    Logger::setOutput(asyncOutput);

    g_asyncLog = log;
    log->start(); // 启动日志写入线程
    printf("Log init success: %s.log\n", name);
    return 0;
}
void deInitLog() {
    if (g_asyncLog) {
        delete g_asyncLog;
        g_asyncLog = nullptr;
    }
}


int main(int argc, char *argv[])
{
    signal(SIGPIPE, SIG_IGN);//ignore SIGPIPE, in case of client close the connection
    printf("pid = %d\n", getpid());

    initLog();

//    CacheManager *cacheManager = CacheManager::getInstance();
//    if(cacheManager==nullptr)
//    {
//        LOG_ERROR<<"CacheManager init failed";
//        return -1;
//    }

    ConfigFileReader config_file("ShareShift.conf");

    char *http_listen_ip = config_file.GetConfigName("HttpListenIP");
    char *str_http_port = config_file.GetConfigName("HttpPort");
    char *dfs_path_client = config_file.GetConfigName("dfs_path_client");
    char *web_server_ip = config_file.GetConfigName("web_server_ip");
    char *web_server_port = config_file.GetConfigName("web_server_port");
    char *storage_web_server_ip =
            config_file.GetConfigName("storage_web_server_ip");
    char *storage_web_server_port =
            config_file.GetConfigName("storage_web_server_port");

    char *str_thread_num = config_file.GetConfigName("ThreadNum");
    uint32_t thread_num = atoi(str_thread_num);

//    ApiUploadInit(dfs_path_client, web_server_ip, web_server_port,
//                  storage_web_server_ip, storage_web_server_port);
//    ret = ApiDealfileInit(dfs_path_client);
    auto mysql_pool = MySqlManager::getInstance();
    deInitLog();

}