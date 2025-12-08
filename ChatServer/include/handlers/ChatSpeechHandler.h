#pragma once

#include "router/RouterHandler.h"
#include "utils/ParseJsonUtil.h"
#include "AIUtil/AISpeechProcessor.h"
#include "AIUtil/AIConfig.h"
#include "ChatServer.h"

class ChatSpeechHandler : public http::router::RouterHandler
{
public:
    explicit ChatSpeechHandler(ChatServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
    ChatServer* server_;
};
