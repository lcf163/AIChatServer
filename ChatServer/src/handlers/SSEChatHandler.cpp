#include "handlers/SSEChatHandler.h"
#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <sstream>

// 静态成员变量定义
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
        resp->setStreamWriteCallback([this, sessionId](muduo::net::TcpConnectionPtr conn, http::HttpResponse* r) {
            // 注册连接
            server_->addSSEConnection(sessionId, conn);
            
            // 发送初始连接确认事件
            std::ostringstream initData;
            initData << "event: connected\n";
            initData << "data: {\"sessionId\": \"" << sessionId << "\", \"status\": \"connected\"}\n\n";
            
            conn->send(initData.str());
            
            return false; // 返回false停止HttpServer的驱动循环，但连接保持打开
        });
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
        
        if (conn->connected()) {
            conn->send(&buf);
        }
        
        // 继续流式传输，等待更多内容或结束标记
        return true;
    }
    
    // 检查连接是否已经断开或者需要关闭
    if (!conn || !conn->connected()) {
        // 清理超时计数器
        {
            std::lock_guard<std::mutex> lock(timeoutMutex_);
            timeoutCounters_.erase(sessionId);
        }
        
        // 清理会话连接映射
        {
            std::lock_guard<std::mutex> lock(server->sseMutex_);
            server->sseConnections_.erase(sessionId);
        }
        
        return false; // 连接已断开，停止流式传输
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
                LOG_INFO << "Timeout reached for session: " << sessionId;
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
        
        // 清理会话连接映射
        {
            std::lock_guard<std::mutex> lock(server->sseMutex_);
            server->sseConnections_.erase(sessionId);
        }
        
        return false; // 超时，停止流式传输
    }
    
    // 暂时没有结果，继续等待
    return true; // 继续流式传输
}
