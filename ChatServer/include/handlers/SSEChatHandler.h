#pragma once

#include "ChatServer.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "router/RouterHandler.h"
#include "utils/JsonUtil.h"
#include <unordered_map>
#include <mutex>
#include <memory>

class SSEChatHandler : public http::router::RouterHandler
{
public:
	SSEChatHandler(ChatServer* server) : server_(server) {}
	void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
	ChatServer* server_;
};