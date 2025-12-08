#pragma once

#include "utils/JsonUtil.h"

#include "router/RouterHandler.h"
#include "ChatServer.h"

class ChatGetResultHandler : public http::router::RouterHandler
{
public:
	ChatGetResultHandler(ChatServer* server) :server_(server) {}
	void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
	ChatServer* server_;
};