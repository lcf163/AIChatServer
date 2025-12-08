#include "handlers/ChatGetResultHandler.h"
#include <chrono>
#include <thread>

void ChatGetResultHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    try
    {
        auto session = server_->getSessionManager()->getSession(req, resp);
        if (session->getValue("isLoggedIn") != "true")
        {
            json errorResp;
            errorResp["status"] = "error";
            errorResp["message"] = "Unauthorized";
            std::string errorBody = errorResp.dump(4);

            server_->packageResp(req.getVersion(), http::HttpResponse::k401Unauthorized,
                "Unauthorized", true, "application/json", errorBody.size(),
                errorBody, resp);
            return;
        }

        std::string sessionId;
        auto body = req.getBody();
        if (!body.empty()) {
            auto j = json::parse(body);
            if (j.contains("sessionId")) sessionId = j["sessionId"];
        }

        if (sessionId.empty()) {
            json errorResp;
            errorResp["status"] = "error";
            errorResp["message"] = "Missing sessionId";
            std::string errorBody = errorResp.dump(4);

            server_->packageResp(req.getVersion(), http::HttpResponse::k400BadRequest,
                "Bad Request", true, "application/json", errorBody.size(),
                errorBody, resp);
            return;
        }

        // 长轮询实现：等待结果最多30秒
        std::string result;
        const int maxWaitSeconds = 30;
        const int checkIntervalMs = 500; // 每500毫秒检查一次
        const int maxChecks = maxWaitSeconds * 1000 / checkIntervalMs;

        for (int i = 0; i < maxChecks; ++i) {
            {
                std::lock_guard<std::mutex> lock(server_->mutexForChatResults);
                auto it = server_->chatResults.find(sessionId);
                if (it != server_->chatResults.end()) {
                    result = it->second;
                    // 从缓存中移除结果，避免重复获取
                    server_->chatResults.erase(it);
                    break;
                }
            }
            
            // 等待一段时间再检查
            std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
        }

        json successResp;
        if (!result.empty()) {
            successResp["success"] = true;
            successResp["result"] = result;
        } else {
            successResp["success"] = false;
            successResp["message"] = "Result not ready yet";
        }

        std::string successBody = successResp.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(successBody.size());
        resp->setBody(successBody);
        return;
    }
    catch (const std::exception& e)
    {
        json failureResp;
        failureResp["status"] = "error";
        failureResp["message"] = e.what();
        std::string failureBody = failureResp.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
    }
}