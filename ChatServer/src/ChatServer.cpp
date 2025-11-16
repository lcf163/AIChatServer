#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "http/HttpServer.h"

#include "handlers/ChatLoginHandler.h"
#include "handlers/ChatRegisterHandler.h"
#include "handlers/ChatLogoutHandler.h"
#include "handlers/ChatHandler.h"
#include "handlers/ChatEntryHandler.h"
#include "handlers/ChatSendHandler.h"
#include "handlers/AIMenuHandler.h"
#include "handlers/ChatHistoryHandler.h"
#include "handlers/ChatCreateAndSendHandler.h"
#include "handlers/ChatSessionsHandler.h"
#include "handlers/ChatSpeechHandler.h"
#include "ChatServer.h"

using namespace http;

ChatServer::ChatServer(int port,
    const std::string& name,
    muduo::net::TcpServer::Option option)
    : httpServer_(port, name, option)
{
    initialize();
}

void ChatServer::initialize() {
    std::cout << "ChatServer initialize start ! " << std::endl;
    
    // 初始化 MysqlUtil
	http::MysqlUtil::init("tcp://127.0.0.1:3306", "root", "root", "ChatHttpServer", 5);
    
    // 初始化 Session（包含会话列表懒加载）
    initializeSession();

    // 初始化中间件
    initializeMiddleware();

    // 初始化路由接口，请求通过 Handler 做相应业务处理
    initializeRouter();

    std::cout << "ChatServer initialize success ! " << std::endl;
}

void ChatServer::loadSessionsFromDatabase() {
    std::cout << "Loading sessions from database..." << std::endl;
    
    try {
        std::string sql = "SELECT user_id, session_id FROM chat_session ORDER BY user_id, created_at";
        sql::ResultSet* result = mysqlUtil_.executeQuery(sql);
        
        if (!result) {
            std::cout << "No sessions found in database (first startup?)" << std::endl;
            return;
        }

        std::lock_guard<std::mutex> lockSessions(mutexForSessionsId);
        std::lock_guard<std::mutex> lockChat(mutexForChatInformation);
        
        int sessionCount = 0;
        while (result->next()) {
            int userId = result->getInt("user_id");
            std::string sessionId = result->getString("session_id");

            // 只加载会话ID列表，不加载消息内容（懒加载策略）
            auto& sessionList = sessionsIdsMap[userId];
            if (std::find(sessionList.begin(), sessionList.end(), sessionId) == sessionList.end()) {
                sessionList.push_back(sessionId);
            }
            
            sessionCount++;
        }
        
        delete result;
        std::cout << "Loaded " << sessionCount << " sessions (messages will load on demand)" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Session loading skipped: " << e.what() << std::endl;
    }
}

// 按需加载会话消息
std::shared_ptr<AIHelper> ChatServer::loadSessionOnDemand(int userId, const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(mutexForChatInformation);
    
    // 检查是否已在内存中
    auto& userSessions = chatInformation[userId];
    auto it = userSessions.find(sessionId);
    if (it != userSessions.end()) {
        return it->second;
    }
    
    // 从数据库加载会话消息
    try {
        std::string sql = "SELECT is_user, content, ts FROM chat_message "
                    "WHERE user_id = " + std::to_string(userId) + 
                    " AND session_id = '" + sessionId + "' "
                    "ORDER BY ts ASC, id ASC";

        sql::ResultSet* result = mysqlUtil_.executeQuery(sql);

        // 创建AIHelper实例
        auto helper = std::make_shared<AIHelper>();
        int messageCount = 0;

        if (result) {
            // 恢复历史消息
            while (result->next()) {
                std::string content = result->getString("content");
                long long ts = result->getInt64("ts");
                helper->restoreMessage(content, ts);
                messageCount++;
            }
            delete result;
        }
        
        // 添加到内存
        userSessions[sessionId] = helper;
        
        // 更新会话列表
        auto& sessionList = sessionsIdsMap[userId];
        if (std::find(sessionList.begin(), sessionList.end(), sessionId) == sessionList.end()) {
            sessionList.push_back(sessionId);
        }
        
        std::cout << "Loaded " << messageCount << " messages for session " << sessionId << std::endl;
        return helper;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to load session " << sessionId << ": " << e.what() << std::endl;
        
        // 创建空会话作为fallback
        auto helper = std::make_shared<AIHelper>();
        userSessions[sessionId] = helper;
        return helper;
    }
}

void ChatServer::setThreadNum(int numThreads) {
    httpServer_.setThreadNum(numThreads);
}

void ChatServer::start() {
    httpServer_.start();
}

void ChatServer::initializeRouter() {
    httpServer_.Get("/", std::make_shared<ChatEntryHandler>(this));
    httpServer_.Get("/entry", std::make_shared<ChatEntryHandler>(this));
    
    httpServer_.Post("/login", std::make_shared<ChatLoginHandler>(this));
    httpServer_.Post("/register", std::make_shared<ChatRegisterHandler>(this));
    httpServer_.Post("/user/logout", std::make_shared<ChatLogoutHandler>(this));

    httpServer_.Get("/chat", std::make_shared<ChatHandler>(this));
    httpServer_.Post("/chat/send", std::make_shared<ChatSendHandler>(this));
    httpServer_.Post("/chat/tts", std::make_shared<ChatSpeechHandler>(this));
    httpServer_.Get("/chat/sessions", std::make_shared<ChatSessionsHandler>(this));
    httpServer_.Post("/chat/history", std::make_shared<ChatHistoryHandler>(this));
    httpServer_.Post("/chat/send-new-session", std::make_shared<ChatCreateAndSendHandler>(this));
 
    httpServer_.Get("/menu", std::make_shared<AIMenuHandler>(this));
}

void ChatServer::initializeSession() {
    auto sessionStorage = std::make_unique<http::session::MemorySessionStorage>();
    auto sessionManager = std::make_unique<http::session::SessionManager>(std::move(sessionStorage));

    setSessionManager(std::move(sessionManager));
    loadSessionsFromDatabase();
}

void ChatServer::initializeMiddleware() {
    auto corsMiddleware = std::make_shared<http::middleware::CorsMiddleware>();

    httpServer_.addMiddleware(corsMiddleware);
}

void ChatServer::packageResp(const std::string& version,
    http::HttpResponse::HttpStatusCode statusCode,
    const std::string& statusMsg,
    bool close,
    const std::string& contentType,
    int contentLen,
    const std::string& body,
    http::HttpResponse* resp)
{
    if (resp == nullptr)
    {
        LOG_ERROR << "Response pointer is null";
        return;
    }

    try
    {
        resp->setVersion(version);
        resp->setStatusCode(statusCode);
        resp->setStatusMessage(statusMsg);
        resp->setCloseConnection(close);
        resp->setContentType(contentType);
        resp->setContentLength(contentLen);
        resp->setBody(body);

        LOG_INFO << "Response packaged successfully";
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Error in packageResp: " << e.what();

        resp->setStatusCode(http::HttpResponse::k500InternalServerError);
        resp->setStatusMessage("Internal Server Error");
        resp->setCloseConnection(true);
    }
}
