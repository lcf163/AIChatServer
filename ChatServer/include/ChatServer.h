#pragma once

#include <atomic>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <list>
#include <mutex>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "http/HttpServer.h"
#include "utils/MysqlUtil.h"
#include "utils/FileUtil.h"

#include "utils/base64.h"
#include "utils/MQManager.h"
#include "utils/ThreadPool.h"
#include "AIUtil/AISpeechProcessor.h"
#include "AIUtil/AIHelper.h"

class ChatLoginHandler;
class ChatRegisterHandler;
class ChatLogoutHandler;
class ChatHandler;
class ChatEntryHandler;
class ChatSendHandler;
class ChatSpeechHandler;
class ChatSessionsHandler;
class ChatHistoryHandler;
class ChatCreateAndSendHandler;
class ChatGetResultHandler;
class SSEChatHandler;

class AIMenuHandler;

class ChatServer {
public:
	ChatServer(int port,
		const std::string& name,
		muduo::net::TcpServer::Option option = muduo::net::TcpServer::kNoReusePort);

	void start();

	void setThreadNum(int numThreads);

	http::session::SessionManager* getSessionManager() const
	{
		return httpServer_.getSessionManager();
	}

	void setSessionManager(std::unique_ptr<http::session::SessionManager> manager)
	{
		httpServer_.setSessionManager(std::move(manager));
	}
	
	// 添加获取server指针的方法，供SSEChatHandler使用
	ChatServer* getServer() { return this; }
	
private:
	friend class ChatLoginHandler;
	friend class ChatRegisterHandler;
	friend class ChatLogoutHandler;
	friend class ChatHandler;
	friend class ChatEntryHandler;
	friend class ChatSendHandler;
	friend class ChatSpeechHandler;
	friend class ChatHistoryHandler;
	friend class ChatSessionsHandler;
	friend class ChatCreateAndSendHandler;
	friend class ChatGetResultHandler;
	friend class SSEChatHandler;
	friend class AIMenuHandler;

private:
	void initialize();
	void initializeSession();
	void initializeRouter();
	void initializeMiddleware();
	
	void loadSessionsFromDatabase();

	// 按需加载会话消息
	std::shared_ptr<AIHelper> loadSessionOnDemand(int userId, const std::string& sessionId);

	void packageResp(const std::string& version, http::HttpResponse::HttpStatusCode statusCode,
		const std::string& statusMsg, bool close, const std::string& contentType,
		int contentLen, const std::string& body, http::HttpResponse* resp);

	// LRU Cache相关方法
	void updateLRUCache(int userId, const std::string& sessionId);
	void evictLRUCacheIfNeeded();

	http::HttpServer httpServer_;

	http::MysqlUtil mysqlUtil_;

	std::unordered_map<int, bool> onlineUsers_;
	std::mutex mutexForOnlineUsers_;

	std::unordered_map<int, std::unordered_map<std::string, std::shared_ptr<AIHelper>>> chatInformation;
	std::mutex mutexForChatInformation;

	std::unordered_map<int, std::vector<std::string>> sessionsIdsMap;
	std::mutex mutexForSessionsId;
	
	// 添加业务线程池，用于处理AI请求
	std::shared_ptr<ThreadPool> businessThreadPool_;
	
	// 存储聊天结果的容器
	std::unordered_map<std::string, std::string> chatResults;
	std::mutex mutexForChatResults;
	
	// LRU Cache相关成员
	std::list<std::pair<int, std::string>> lruCacheList_;
	std::unordered_map<std::string, std::list<std::pair<int, std::string>>::iterator> lruCacheMap_;
	std::mutex lruCacheMutex_;
	static const size_t MAX_ACTIVE_SESSIONS = 1000;
};