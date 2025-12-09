#pragma once

#include <unordered_map>
#include <mutex>
#include <memory>
#include <muduo/net/TcpConnection.h>

#include "utils/JsonUtil.h"

#include "router/RouterHandler.h"
#include "ChatServer.h"

class SSEChatHandler : public http::router::RouterHandler
{
public:
	static const int SSE_TIMEOUT_CHECKS = 60;  // 60次检查，每次0.5秒，总共30秒
	static const int SSE_CHECK_INTERVAL = 500; // 毫秒
	
	SSEChatHandler(ChatServer* server) : server_(server) {}
	void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
	ChatServer* server_;
	
	// 用于跟踪每个连接的超时计数器
	static std::unordered_map<std::string, int> timeoutCounters_;
	static std::mutex timeoutMutex_;
};