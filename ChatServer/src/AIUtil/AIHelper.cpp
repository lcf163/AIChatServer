#include <chrono>
#include <muduo/base/Logging.h>

#include "utils/MQManager.h"
#include "AIUtil/AIHelper.h"

enum modelType {
    AliType = 1,
    DouType = 2,
    AliRAGType = 3,
    AliMCPType = 4
};

// 构造函数
AIHelper::AIHelper() {
    // 默认使用阿里大模型
    strategy = StrategyFactory::instance().create(std::to_string(AliType));
}

void AIHelper::setStrategy(std::shared_ptr<AIStrategy> strat) {
    strategy = strat;
}

// 添加一条用户消息
void AIHelper::addMessage(int userId, const std::string& userName, bool is_user,const std::string& userInput, std::string sessionId) {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    messages.push_back({ userInput,ms });
    //消息队列异步入库
    pushMessageToMysql(userId, userName, is_user, userInput, ms, sessionId);
}

void AIHelper::restoreMessage(const std::string& userInput,long long ms) {
    messages.push_back({ userInput,ms });
}

// 发送聊天消息
std::string AIHelper::chat(int userId,std::string userName, std::string sessionId, std::string userQuestion, std::string modelType) {
    return chatImpl(userId, userName, sessionId, userQuestion, modelType);
}

// 异步发送聊天消息
std::future<std::string> AIHelper::chatAsync(std::shared_ptr<ThreadPool> pool, int userId, std::string userName, std::string sessionId, std::string userQuestion, std::string modelType) {
    // 使用线程池执行任务
    return pool->enqueue([this, userId, userName, sessionId, userQuestion, modelType]() {
        return chatImpl(userId, userName, sessionId, userQuestion, modelType);
    });
}

// 实际执行聊天逻辑的方法
std::string AIHelper::chatImpl(int userId, std::string userName, std::string sessionId, std::string userQuestion, std::string modelType) {
    // 设置策略
    setStrategy(StrategyFactory::instance().create(modelType));
    
    if (!strategy->isMCPModel) {
        addMessage(userId, userName, true, userQuestion, sessionId);
        json payload = strategy->buildRequest(this->messages);

        // 执行请求
        json response = executeCurl(payload);
        std::string answer = strategy->parseResponse(response);
        addMessage(userId, userName, false, answer, sessionId);
        return answer.empty() ? "[Error] 无法解析响应" : answer;
    }
    
    // 使用单例模式的AIConfig，避免重复加载配置文件
    AIConfig& config = AIConfig::getInstance();
    std::string tempUserQuestion = config.buildPrompt(userQuestion);
    messages.push_back({ tempUserQuestion, 0 });

    json firstReq = strategy->buildRequest(this->messages);
    json firstResp = executeCurl(firstReq);
    std::string aiResult = strategy->parseResponse(firstResp);
    // 用完立即移除提示词
    messages.pop_back();

    // 解析AI响应（是否工具调用）
    AIToolCall call = config.parseAIResponse(aiResult);

    // 情况1：AI 不调用工具
    if (!call.isToolCall) {
        addMessage(userId, userName, true, userQuestion, sessionId);
        addMessage(userId, userName, false, aiResult, sessionId);
        return aiResult;
    }

    // 情况2：AI 调用工具
    AIToolRegistry registry;

    // 验证工具参数
    if (!registry.validateToolArguments(call.toolName, call.args)) {
        std::string prompt = "工具 " + call.toolName + " 缺少必要参数，请提供完整信息。";
        
        // 对特定工具提供更友好的提示
        if (call.toolName == "get_weather") {
            prompt = "我需要知道您想查询哪个城市的天气，请告诉我城市名称。";
        }
        
        addMessage(userId, userName, true, userQuestion, sessionId);
        addMessage(userId, userName, false, prompt, sessionId);
        return prompt;
    }

    // 调用工具并处理结果
    try {
        json toolResult = registry.invoke(call.toolName, call.args);
        
        // 检查工具调用是否返回错误
        if (toolResult.contains("error")) {
            std::string errorMsg = toolResult["error"];
            std::string errMsg = "抱歉，" + errorMsg + "。请您稍后再试，或者通过其他渠道查询。";
            addMessage(userId, userName, true, userQuestion, sessionId);
            addMessage(userId, userName, false, errMsg, sessionId);
            return errMsg;
        }
        
        // 构建工具调用结果的提示词
        std::string secondPrompt = config.buildToolResultPrompt(userQuestion, call.toolName, call.args, toolResult);
        messages.push_back({ secondPrompt, 0 });

        json secondReq = strategy->buildRequest(messages);
        json secondResp = executeCurl(secondReq);
        std::string finalAnswer = strategy->parseResponse(secondResp);
        // 删除包含提示词的信息
        messages.pop_back();

        addMessage(userId, userName, true, userQuestion, sessionId);
        addMessage(userId, userName, false, finalAnswer, sessionId);
        return finalAnswer;
    }
    catch (const std::exception& e) {
        std::string err = "[工具调用失败] " + std::string(e.what());
        addMessage(userId, userName, true, userQuestion, sessionId);
        addMessage(userId, userName, false, err, sessionId);
        return err;
    }
}

// 发送自定义请求体
json AIHelper::request(const json& payload) {
    return executeCurl(payload);
}

std::vector<std::pair<std::string, long long>> AIHelper::GetMessages() {
    return this->messages;
}

// 执行 curl 请求
json AIHelper::executeCurl(const json& payload) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize curl");
    }

    LOG_INFO << "AI API invoke " << strategy->getApiUrl().c_str() << ' ' << strategy->getApiKey();

    std::string readBuffer;
    struct curl_slist* headers = nullptr;
    std::string authHeader = "Authorization: Bearer " + strategy->getApiKey();

    headers = curl_slist_append(headers, authHeader.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string payloadStr = payload.dump();

    curl_easy_setopt(curl, CURLOPT_URL, strategy->getApiUrl().c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
    }

    try {
        return json::parse(readBuffer);
    }
    catch (...) {
        throw std::runtime_error("Failed to parse JSON response: " + readBuffer);
    }
}

// curl 回调函数，把返回的数据写到 string buffer
size_t AIHelper::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::string AIHelper::escapeString(const std::string& input) {
    std::string output;
    output.reserve(input.size() * 2);
    for (char c : input) {
        switch (c) {
            case '\\': output += "\\\\"; break;
            case '\'': output += "\\\'"; break;
            case '\"': output += "\\\""; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default:   output += c; break;
        }
    }
    return output;
}

void AIHelper::pushMessageToMysql(int userId, const std::string& userName, bool is_user, const std::string& userInput, long long ms, std::string sessionId) {
    // 构造带参数的SQL消息
    std::string sql = "INSERT INTO chat_message (user_id, username, session_id, is_user, content, ts) VALUES (?, ?, ?, ?, ?, ?)";
    
    // 构造参数列表，使用特殊分隔符分隔SQL和参数
    std::string params = std::to_string(userId) + "|~|" + userName + "|~|" + sessionId + "|~|" + 
                        std::to_string(is_user ? 1 : 0) + "|~|" + userInput + "|~|" + std::to_string(ms);
    std::string message = sql + "|~|" + params;  // 使用|~|作为分隔符
    
    // 消息队列异步执行mysql操作，用于流量削峰与解耦逻辑
    MQManager::instance().publish("sql_queue", message);
}