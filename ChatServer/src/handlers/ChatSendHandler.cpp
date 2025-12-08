#include "handlers/ChatSendHandler.h"

void ChatSendHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
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
		std::string sessionId;
		std::string modelType;

		auto body = req.getBody();
		if (!body.empty()) {
			auto j = json::parse(body);
			if (j.contains("question")) userQuestion = j["question"];
			if (j.contains("sessionId")) sessionId = j["sessionId"];

			modelType = j.contains("modelType") ? j["modelType"].get<std::string>() : "1";
		}

		std::shared_ptr<AIHelper> AIHelperPtr;
		{
			std::lock_guard<std::mutex> lock(server_->mutexForChatInformation);
			auto& userSessions = server_->chatInformation[userId];

			// 直接获取或创建AIHelper实例
			if (userSessions.find(sessionId) == userSessions.end()) {
				// 第一次访问：创建新实例
				AIHelperPtr = std::make_shared<AIHelper>();
				userSessions[sessionId] = AIHelperPtr;
			} else {
				// 已存在：直接获取
				AIHelperPtr = userSessions[sessionId];
			}
			
			// 更新LRU缓存
			server_->updateLRUCache(userId, sessionId);
		}

		// 更新会话时间戳
		try {
			// 使用参数化查询防止SQL注入
			std::string updateSessionSql = "UPDATE chat_session SET updated_at = CURRENT_TIMESTAMP WHERE session_id = ?";
			mysqlUtil_.executeUpdate(updateSessionSql, sessionId);
		} catch (const std::exception& e) {
			LOG_ERROR << "Failed to update session timestamp: " << e.what();
			// 继续执行，即使数据库操作失败
		}

		// 使用线程池异步处理AI请求，真正避免阻塞IO线程
		auto future = AIHelperPtr->chatAsync(server_->businessThreadPool_, userId, username, sessionId, userQuestion, modelType);
		
		// 在另一个线程中等待结果并处理
		server_->businessThreadPool_->enqueue([this, sessionId, userId, future = std::move(future)]() mutable {
			try {
				std::string result = future.get();
				// 通过WebSocket推送结果给客户端
				std::lock_guard<std::mutex> lock(server_->mutexForChatResults);
				server_->chatResults[sessionId] = result;
			} catch (const std::exception& e) {
				std::cerr << "AI task failed for session " << sessionId << ": " << e.what() << std::endl;
			}
		});

		// 立即返回成功响应，前端需轮询结果
		json successResp;
		successResp["success"] = true;
		successResp["message"] = "AI processing started";

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
		failureResp["success"] = false;
		failureResp["error"] = e.what();
		std::string failureBody = failureResp.dump(4);

		resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
		resp->setCloseConnection(true);
		resp->setContentType("application/json");
		resp->setContentLength(failureBody.size());
		resp->setBody(failureBody);
		return;
	}
}