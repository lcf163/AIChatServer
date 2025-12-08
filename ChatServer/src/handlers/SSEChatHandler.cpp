#include "handlers/SSEChatHandler.h"
#include <sstream>
#include <chrono>
#include <thread>
#include <muduo/net/Buffer.h>

// 超时检查次数（每次0.5秒，60次共30秒）
constexpr int SSE_TIMEOUT_CHECKS = 60;

// 初始化静态成员
std::unordered_map<std::string, int> SSEChatHandler::timeoutCounters_;
std::mutex SSEChatHandler::timeoutMutex_;

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

        // 初始化超时计数器
        {
            std::lock_guard<std::mutex> lock(timeoutMutex_);
            timeoutCounters_[sessionId] = SSE_TIMEOUT_CHECKS;
        }

        // 设置SSE响应头
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->addHeader("Content-Type", "text/event-stream");
        resp->addHeader("Cache-Control", "no-cache");
        resp->addHeader("Connection", "keep-alive");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("X-Accel-Buffering", "no"); // 禁用nginx缓冲
        
        // 启用流式响应模式
        resp->setStreamingMode(true);
        resp->setStreamWriteCallback([this](muduo::net::TcpConnectionPtr conn, http::HttpResponse* r) {
            return streamWriteCallback(conn, r, server_);
        });
        
        // 存储会话ID到响应对象中（可以通过某种方式传递，这里简化处理）
        resp->addHeader("X-SSE-Session-ID", sessionId);
        
        // 发送初始连接确认事件
        std::ostringstream initData;
        initData << "event: connected\n";
        initData << "data: {\"sessionId\": \"" << sessionId << "\", \"status\": \"connected\"}\n\n";
        
        muduo::net::Buffer buf;
        buf.append(initData.str());
        // 注意：初始数据将在onRequest中发送
    }
    catch (const std::exception& e)
    {
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k500InternalServerError, "Internal Server Error");
        resp->setCloseConnection(true);
    }
}

bool SSEChatHandler::streamWriteCallback(muduo::net::TcpConnectionPtr conn, http::HttpResponse* resp, ChatServer* server) {
    if (!conn || !conn->connected()) {
        return false; // 停止流式传输
    }
    
    std::string sessionId = resp->getHeader("X-SSE-Session-ID");
    if (sessionId.empty()) {
        return false; // 停止流式传输
    }
    
    // 检查是否有结果
    std::string result;
    bool foundResult = false;
    {
        std::lock_guard<std::mutex> lock(server->mutexForChatResults);
        auto it = server->chatResults.find(sessionId);
        if (it != server->chatResults.end()) {
            result = it->second;
            // 从缓存中移除结果，避免重复获取
            server->chatResults.erase(it);
            foundResult = true;
        }
    }
    
    muduo::net::Buffer buf;
    if (foundResult) {
        // 发送结果事件
        std::ostringstream eventData;
        eventData << "event: result\n";
        json resultData = {{"result", result}};
        eventData << "data: " << resultData.dump() << "\n\n";
        buf.append(eventData.str());
        
        // 发送结束事件
        buf.append("event: end\n");
        buf.append("data: {\"message\": \"connection closing\"}\n\n");
        
        // 清理超时计数器
        {
            std::lock_guard<std::mutex> lock(timeoutMutex_);
            timeoutCounters_.erase(sessionId);
        }
        
        if (conn->connected()) {
            conn->send(&buf);
        }
        return false; // 完成传输，停止流式传输
    }
    
    // 检查是否超时
    bool timeout = false;
    {
        std::lock_guard<std::mutex> lock(timeoutMutex_);
        auto it = timeoutCounters_.find(sessionId);
        if (it != timeoutCounters_.end()) {
            if (--(it->second) <= 0) {
                timeout = true;
                timeoutCounters_.erase(it);
                std::cout << "Timeout reached for session: " << sessionId << std::endl;
            } else {
                // std::cout << "Remaining checks for session " << sessionId << ": " << it->second << std::endl;
            }
        } else {
            // 如果找不到计数器，也认为超时
            timeout = true;
        }
    }
    
    if (timeout) {
        // 发送超时事件
        buf.append("event: timeout\n");
        buf.append("data: {\"message\": \"AI processing timeout\"}\n\n");
        
        // 发送结束事件
        buf.append("event: end\n");
        buf.append("data: {\"message\": \"connection closing\"}\n\n");
        
        if (conn->connected()) {
            conn->send(&buf);
        }
        return false; // 超时，停止流式传输
    }
    
    // 暂时没有结果，继续等待
    return true; // 继续流式传输
}