#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include "ChatServer.h"
#include "AIUtil/AIConfig.h"

// 工作线程绑定的函数
void executeMysql(const std::string sql_with_params) {
    // 从配置中获取数据库信息
    const auto& dbConfig = AIConfig::getInstance().getDatabaseConfig();
    
    http::MysqlUtil mysqlUtil_;
    
    // 解析SQL和参数
    size_t pos = sql_with_params.find("|~|");
    if (pos != std::string::npos) {
        // 分离SQL语句和参数
        std::string sql = sql_with_params.substr(0, pos);
        std::string params_str = sql_with_params.substr(pos + 3);

        // 分割参数
        std::vector<std::string> params;
        size_t start = 0;
        size_t end = 0;
        while ((end = params_str.find("|~|", start)) != std::string::npos) {
            params.push_back(params_str.substr(start, end - start));
            start = end + 3;
        }
        params.push_back(params_str.substr(start));

        // 根据参数数量执行相应的executeUpdate
        try {
            switch (params.size()) {
                case 1:
                    mysqlUtil_.executeUpdate(sql, params[0]);
                    break;
                case 2:
                    mysqlUtil_.executeUpdate(sql, params[0], params[1]);
                    break;
                case 3:
                    mysqlUtil_.executeUpdate(sql, params[0], params[1], params[2]);
                    break;
                case 4:
                    mysqlUtil_.executeUpdate(sql, params[0], params[1], params[2], params[3]);
                    break;
                case 5:
                    mysqlUtil_.executeUpdate(sql, params[0], params[1], params[2], params[3], params[4]);
                    break;
                case 6:
                    mysqlUtil_.executeUpdate(sql, params[0], params[1], params[2], params[3], params[4], params[5]);
                    break;
                default:
                    // 如果参数数量不匹配，则直接执行原始SQL（向后兼容）
                    mysqlUtil_.executeUpdate(sql_with_params);
                    break;
            }
        } catch (const std::exception& e) {
            LOG_ERROR << "Error executing SQL: " << e.what();
        }
    } else {
        // 如果没有找到分隔符，则直接执行原始SQL（向后兼容）
        try {
            mysqlUtil_.executeUpdate(sql_with_params);
        } catch (const std::exception& e) {
            LOG_ERROR << "Error executing SQL: " << e.what();
        }
    }
}

int main(int argc, char* argv[]) {
	std::string serverName = "ChatServer";
	int port = 8080;
    int opt;
    const char* str = "p:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            port = atoi(optarg);
            break;
        }
        default:
            break;
        }
    }

    // 加载配置文件
    AIConfig& config = AIConfig::getInstance();
    if (!config.loadFromFile("../ChatServer/resource/config.json")) {
        LOG_ERROR << "Failed to load config file!";
        return -1;
    }
    
    // 根据配置设置日志级别
    const auto& logConfig = config.getLogConfig();
    switch (logConfig.level) {
        case LogLevel::TRACE:
            muduo::Logger::setLogLevel(muduo::Logger::TRACE);
            break;
        case LogLevel::DEBUG:
            muduo::Logger::setLogLevel(muduo::Logger::DEBUG);
            break;
        case LogLevel::INFO:
            muduo::Logger::setLogLevel(muduo::Logger::INFO);
            break;
        case LogLevel::WARN:
            muduo::Logger::setLogLevel(muduo::Logger::WARN);
            break;
        case LogLevel::ERROR:
            muduo::Logger::setLogLevel(muduo::Logger::ERROR);
            break;
        case LogLevel::FATAL:
            muduo::Logger::setLogLevel(muduo::Logger::FATAL);
            break;
        default:
            muduo::Logger::setLogLevel(muduo::Logger::WARN);
            break;
    }
    
    // 在设置日志级别后再打印进程ID
    LOG_INFO << "pid = " << getpid();

    // 初始化 ChatServer
    ChatServer server(port, serverName);
    // server.setThreadNum(4);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 从配置中获取RabbitMQ信息
    const auto& mqConfig = config.getRabbitMQConfig();
    
    // 初始化线程池，绑定参数
    RabbitMQThreadPool pool(mqConfig.host, mqConfig.queueName, mqConfig.threadNum, executeMysql);
    // 启动线程池，作为消费者处理 MQ 的消息
    pool.start();

    server.start();
    
    return 0;
}