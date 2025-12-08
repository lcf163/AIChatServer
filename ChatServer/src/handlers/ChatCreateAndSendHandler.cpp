#include "handlers/ChatCreateAndSendHandler.h"

void ChatCreateAndSendHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
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

		// 持久化会话到数据库
		try {
			std::string insertSessionSql = std::string("INSERT INTO chat_session (user_id, username, session_id, title) VALUES (")
				+ std::to_string(userId) + ", "
				+ "'" + username + "', "
				+ "'" + sessionId + "', "
				+ "'新对话')";
			
			mysqlUtil_.executeUpdate(insertSessionSql);
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
			
			// 更新LRU缓存
			server_->updateLRUCache(userId, sessionId);
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

		// 立即返回 sessionId，前端需轮询结果
		json successResp;
		successResp["success"] = true;
		successResp["message"] = "AI processing started";
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