#include "utils/ParseJsonUtil.h"

bool ParseJsonUtil::parseJsonFromBody(const http::HttpRequest& req, http::HttpResponse* resp, json& outJson) {
    // 检查Content-Type头部和请求体
    auto contentType = req.getHeader("Content-Type");
    if (req.getBody().empty() || (contentType != "application/json" && !contentType.empty())) {
        json errorResp;
        errorResp["success"] = false;
        errorResp["error"] = "Invalid request: Content-Type must be application/json";
        std::string errorBody = errorResp.dump(4);
        
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(errorBody.size());
        resp->setBody(errorBody);
        return false;
    }
    
    try {
        outJson = json::parse(req.getBody());
        return true;
    } catch (const std::exception& e) {
        // JSON解析失败时返回400错误
        json errorResp;
        errorResp["success"] = false;
        errorResp["error"] = "Invalid JSON format: " + std::string(e.what());
        std::string errorBody = errorResp.dump(4);
        
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(errorBody.size());
        resp->setBody(errorBody);
        return false;
    }
}