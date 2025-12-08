#pragma once

#include "ChatServer.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "utils/JsonUtil.h"
#include "utils/MysqlUtil.h"
#include "router/RouterHandler.h"
#include "AIUtil/AISessionIdGenerator.h"

class ChatCreateAndSendHandler : public http::router::RouterHandler
{
public:
	ChatCreateAndSendHandler(ChatServer* server) :server_(server) {}
	void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
	ChatServer* server_;
	http::MysqlUtil mysqlUtil_;
};