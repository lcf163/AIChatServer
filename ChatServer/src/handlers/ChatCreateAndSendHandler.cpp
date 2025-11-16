#include "handlers/ChatCreateAndSendHandler.h"

void ChatCreateAndSendHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    try
    {
        auto session = server_->getSessionManager()->getSession(req, resp);
        LOG_INFO << "session->getValue(\"isLoggedIn\") = " << session->getValue("isLoggedIn");
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

        int userId = std::stoi(session->getValue("userId"));
        std::string username = session->getValue("username");
        std::string userQuestion;
        std::string modelType;

        auto body = req.getBody();
        if (!body.empty()) {
            auto j = json::parse(body);
            if (j.contains("question")) userQuestion = j["question"];

            modelType = j.contains("modelType") ? j["modelType"].get<std::string>() : "1";
        }

        AISessionIdGenerator generator;
        std::string sessionId = generator.generate();
        std::cout<< "sessionId: " << sessionId << std::endl;

        // 持久化会话到数据库
        try {
            std::string insertSessionSql = std::string("INSERT INTO chat_session (user_id, username, session_id, title) VALUES (")
                + std::to_string(userId) + ", "
                + "'" + username + "', "
                + "'" + sessionId + "', "
                + "'新对话')";
            
            mysqlUtil_.executeUpdate(insertSessionSql);
            std::cout << "Session " << sessionId << " persisted to database" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Failed to persist session to database: " << e.what() << std::endl;
            // 继续执行，即使数据库操作失败
        }

        std::shared_ptr<AIHelper> AIHelperPtr;
        {
            std::lock_guard<std::mutex> lock(server_->mutexForChatInformation);
            auto& userSessions = server_->chatInformation[userId];

            if (userSessions.find(sessionId) == userSessions.end()) {
                userSessions.emplace( 
                    sessionId,
                    std::make_shared<AIHelper>()
                );
                server_->sessionsIdsMap[userId].push_back(sessionId);
            }
            AIHelperPtr= userSessions[sessionId];
        }

        std::string aiInformation=AIHelperPtr->chat(userId, username,sessionId, userQuestion, modelType);
        json successResp;
        successResp["success"] = true;
        successResp["Information"] = aiInformation;
        successResp["sessionId"] = sessionId;
        
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
