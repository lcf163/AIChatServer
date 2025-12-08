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
#include "handlers/ChatGetResultHandler.h"
#include "handlers/SSEChatHandler.h"
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

    // 初始化业务线程池，用于处理AI请求
    businessThreadPool_ = std::make_shared<ThreadPool>(4);

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
        // 如果已存在，更新LRU缓存并返回
        updateLRUCache(userId, sessionId);
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
        
        // 加载成功后，更新LRU缓存
        updateLRUCache(userId, sessionId);
        
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
    // httpServer_.Post("/chat/get-result", std::make_shared<ChatGetResultHandler>(this)); // 已废弃，改用SSE
    httpServer_.Get("/chat/stream", std::make_shared<SSEChatHandler>(this));
 
    httpServer_.Get("/menu", std::make_shared<AIMenuHandler>(this));
}

void ChatServer::initializeSession() {
    auto sessionStorage = std::make_unique<http::session::MemorySessionStorage>();
    auto sessionManager = std::make_unique<http::session::SessionManager>(std::move(sessionStorage));

    setSessionManager(std::move(sessionManager));
    loadSessionsFromDatabase();
}

void ChatServer::updateLRUCache(int userId, const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(lruCacheMutex_);
    
    // 如果会话已在LRU缓存中，先移除旧条目
    auto it = lruCacheMap_.find(sessionId);
    if (it != lruCacheMap_.end()) {
        lruCacheList_.erase(it->second);
        lruCacheMap_.erase(it);
    }
    
    // 将新会话添加到LRU链表头部（表示最近使用）
    lruCacheList_.emplace_front(userId, sessionId);
    lruCacheMap_[sessionId] = lruCacheList_.begin();
    
    // 检查并执行LRU淘汰策略
    evictLRUCacheIfNeeded();
}

void ChatServer::evictLRUCacheIfNeeded() {
    // 当活跃会话数超过最大限制时，开始淘汰
    while (lruCacheList_.size() > MAX_ACTIVE_SESSIONS) {
        // 获取并移除LRU链表尾部元素（最久未使用的会话）
        auto last = lruCacheList_.back();
        int userId = last.first;
        std::string sessionId = last.second;
        
        lruCacheMap_.erase(sessionId);
        lruCacheList_.pop_back();
        
        // 从内存中的聊天信息里移除该会话
        auto userIt = chatInformation.find(userId);
        if (userIt != chatInformation.end()) {
            userIt->second.erase(sessionId);
            // 如果该用户的所有会话都被移除，则清理用户条目
            if (userIt->second.empty()) {
                chatInformation.erase(userIt);
            }
        }
        
        std::cout << "LRU Cache Eviction: Removed session '" << sessionId 
                  << "' for user " << userId << std::endl;
    }
}

void ChatServer::initializeMiddleware() {
    // 添加 CORS 中间件
    auto corsMiddleware = std::make_shared<http::middleware::CorsMiddleware>();
    httpServer_.addMiddleware(corsMiddleware);
}

void ChatServer::packageResp(const std::string& version, http::HttpResponse::HttpStatusCode statusCode,
	const std::string& statusMsg, bool close, const std::string& contentType,
	int contentLen, const std::string& body, http::HttpResponse* resp) {

	resp->setStatusLine(version, statusCode, statusMsg);
	resp->setCloseConnection(close);
	resp->setContentType(contentType);
	resp->setContentLength(contentLen);
	resp->setBody(body);
}
