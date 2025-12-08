#pragma once

#include "ChatServer.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "utils/JsonUtil.h"
#include "router/RouterHandler.h"

class ChatGetResultHandler : public http::router::RouterHandler
{
public:
	ChatGetResultHandler(ChatServer* server) :server_(server) {}
	void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
	ChatServer* server_;
};