#include "AIUtil/AIStrategy.h"
#include "AIUtil/AIFactory.h"

std::string AliyunStrategy::getApiUrl() const {
    return apiUrl_;
}

std::string AliyunStrategy::getApiKey()const {
    return apiKey_;
}

std::string AliyunStrategy::getModel() const {
    return modelName_;
}

json AliyunStrategy::buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const {
    json payload;
    payload["model"] = getModel();
    json msgArray = json::array();

    for (size_t i = 0; i < messages.size(); ++i) {
        json msg;
        if (i % 2 == 0) {
            msg["role"] = "user";
        }
        else {
            msg["role"] = "assistant";
        }
        msg["content"] = messages[i].first;
        msgArray.push_back(msg);
    }
    payload["messages"] = msgArray;
    return payload;
}

std::string AliyunStrategy::parseResponse(const json& response) const {
    if (response.contains("choices") && !response["choices"].empty()) {
        return response["choices"][0]["message"]["content"];
    }
    return {};
}


std::string DouBaoStrategy::getApiUrl()const {
    return apiUrl_;
}

std::string DouBaoStrategy::getApiKey()const {
    return apiKey_;
}

std::string DouBaoStrategy::getModel() const {
    return modelName_;
}

json DouBaoStrategy::buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const {
    json payload;
    payload["model"] = getModel();
    json msgArray = json::array();

    for (size_t i = 0; i < messages.size(); ++i) {
        json msg;
        if (i % 2 == 0) {
            msg["role"] = "user";
        }
        else {
            msg["role"] = "assistant";
        }
        msg["content"] = messages[i].first;
        msgArray.push_back(msg);
    }
    payload["messages"] = msgArray;
    return payload;
}

std::string DouBaoStrategy::parseResponse(const json& response) const {
    if (response.contains("choices") && !response["choices"].empty()) {
        return response["choices"][0]["message"]["content"];
    }
    return {};
}


std::string AliyunRAGStrategy::getApiUrl() const {
    return apiUrlPrefix_ + knowledgeBaseId_ + apiUrlSuffix_;
}

std::string AliyunRAGStrategy::getApiKey()const {
    return apiKey_;
}

std::string AliyunRAGStrategy::getModel() const {
    return "";
}

json AliyunRAGStrategy::buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const {
    json payload;
    json msgArray = json::array();
    for (size_t i = 0; i < messages.size(); ++i) {
        json msg;
        msg["role"] = (i % 2 == 0 ? "user" : "assistant");
        msg["content"] = messages[i].first;
        msgArray.push_back(msg);
    }
    payload["input"]["messages"] = msgArray;
    payload["parameters"] = json::object(); 
    return payload;
}

std::string AliyunRAGStrategy::parseResponse(const json& response) const {
    if (response.contains("output") && response["output"].contains("text")) {
        return response["output"]["text"];
    }
    return {};
}


std::string AliyunMcpStrategy::getApiUrl() const {
    return apiUrl_;
}

std::string AliyunMcpStrategy::getApiKey()const {
    return apiKey_;
}

std::string AliyunMcpStrategy::getModel() const {
    return modelName_;
}

json AliyunMcpStrategy::buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const {
    json payload;
    payload["model"] = getModel();
    json msgArray = json::array();

    for (size_t i = 0; i < messages.size(); ++i) {
        json msg;
        if (i % 2 == 0) {
            msg["role"] = "user";
        }
        else {
            msg["role"] = "assistant";
        }
        msg["content"] = messages[i].first;
        msgArray.push_back(msg);
    }
    payload["messages"] = msgArray;
    return payload;
}

std::string AliyunMcpStrategy::parseResponse(const json& response) const {
    if (response.contains("choices") && !response["choices"].empty()) {
        return response["choices"][0]["message"]["content"];
    }
    return {};
}

static StrategyRegister<AliyunStrategy> regAliyun("1");
static StrategyRegister<DouBaoStrategy> regDoubao("2");
static StrategyRegister<AliyunRAGStrategy> regAliyunRag("3");
static StrategyRegister<AliyunMcpStrategy> regAliyunMcp("4");