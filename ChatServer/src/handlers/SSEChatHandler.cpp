#include "handlers/SSEChatHandler.h"
#include <sstream>
#include <chrono>
#include <thread>
#include <muduo/net/Buffer.h>

void SSEChatHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    try
    {
        auto session = server_->getSessionManager()->getSession(req, resp);
        if (session->getValue("isLoggedIn") != "true")
        {
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k401Unauthorized, "Unauthorized");
            resp->setCloseConnection(true);
            return;
        }

        std::string sessionId = req.getQueryParameters("sessionId");

        if (sessionId.empty()) {
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
            resp->setCloseConnection(true);
            return;
        }

        // 构建响应体
        std::ostringstream responseBody;
        
        // 发送初始连接确认事件
        responseBody << "event: connected\n";
        responseBody << "data: {\"sessionId\": \"" << sessionId << "\", \"status\": \"connected\"}\n\n";
        
        // 等待结果（模拟SSE行为），但不阻塞太久
        std::string result;
        const int maxWaitSeconds = 30; // 等待30秒，给AI足够的时间
        const int checkIntervalMs = 500; // 每500毫秒检查一次
        const int maxChecks = maxWaitSeconds * 1000 / checkIntervalMs;
        bool foundResult = false;

        for (int i = 0; i < maxChecks; ++i) {
            {
                std::lock_guard<std::mutex> lock(server_->mutexForChatResults);
                auto it = server_->chatResults.find(sessionId);
                if (it != server_->chatResults.end()) {
                    result = it->second;
                    // 从缓存中移除结果，避免重复获取
                    server_->chatResults.erase(it);
                    foundResult = true;
                    break;
                }
            }
            
            // 等待一段时间再检查
            std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
        }

        if (foundResult) {
            // 发送结果事件
            responseBody << "event: result\n";
            json resultData = {{"result", result}};
            responseBody << "data: " << resultData.dump() << "\n\n";
        } else {
            // 发送超时事件
            responseBody << "event: timeout\n";
            responseBody << "data: {\"message\": \"AI processing timeout\"}\n\n";
        }
        
        // 发送结束事件
        responseBody << "event: end\n";
        responseBody << "data: {\"message\": \"connection closing\"}\n\n";
        
        // 设置响应
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->addHeader("Content-Type", "text/event-stream");
        resp->addHeader("Cache-Control", "no-cache");
        resp->addHeader("Connection", "keep-alive");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("X-Accel-Buffering", "no"); // 禁用nginx缓冲
        resp->setBody(responseBody.str());
    }
    catch (const std::exception& e)
    {
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k500InternalServerError, "Internal Server Error");
        resp->setCloseConnection(true);
    }
}