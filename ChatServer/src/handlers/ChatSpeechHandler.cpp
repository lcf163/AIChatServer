#include "handlers/ChatSpeechHandler.h"

void ChatSpeechHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    try
    {
        std::string text = "欢迎使用AI智能助手";
        json j;
        if (!ParseJsonUtil::parseJsonFromBody(req, resp, j)) {
            // 错误响应已经在parseJsonFromBody中设置
            return;
        }
        
        if (!j.empty()) {
            if (j.contains("text")) text = j["text"];
        }

        // 从配置中获取百度API密钥
        const auto& apiKeys = AIConfig::getInstance().getApiKeysConfig();
        std::string clientSecret, clientId;
        
        if (!apiKeys.baiduClientSecret.empty()) {
            clientSecret = apiKeys.baiduClientSecret;
        } else {
            const char* secretEnv = std::getenv("BAIDU_CLIENT_SECRET");
            if (!secretEnv) throw std::runtime_error("BAIDU_CLIENT_SECRET not found!");
            clientSecret = std::string(secretEnv);
        }
        
        if (!apiKeys.baiduClientId.empty()) {
            clientId = apiKeys.baiduClientId;
        } else {
            const char* idEnv = std::getenv("BAIDU_CLIENT_ID");
            if (!idEnv) throw std::runtime_error("BAIDU_CLIENT_ID not found!");
            clientId = std::string(idEnv);
        }
        // 创建语音处理器
        AISpeechProcessor speechProcessor(clientId, clientSecret);
        // 获取语音合成后的地址
        std::string speechUrl = speechProcessor.synthesize(text,
                                                           "mp3-16k", 
                                                           "zh",  
                                                            5, 
                                                            5, 
                                                            5 );  

        json successResp;
        successResp["success"] = true;
        successResp["url"] = speechUrl;
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