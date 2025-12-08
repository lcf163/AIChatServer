#include "AIUtil/AIStrategy.h"
#include "AIUtil/AIFactory.h"

// AliyunStrategy implementation
AliyunStrategy::AliyunStrategy() {
    const auto& apiKeys = AIConfig::getInstance().getApiKeysConfig();
    const auto& modelConfig = AIConfig::getInstance().getModelConfig();
    
    if (!apiKeys.dashscopeApiKey.empty()) {
        apiKey_ = apiKeys.dashscopeApiKey;
    } else {
        const char* key = std::getenv("DASHSCOPE_API_KEY");
        if (!key) throw std::runtime_error("Aliyun API Key not found!");
        apiKey_ = std::string(key);
    }
    
    apiUrl_ = modelConfig.aliyun.apiUrl;
    modelName_ = modelConfig.aliyun.modelName;
    isMCPModel = false;
}

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

// DouBaoStrategy implementation
DouBaoStrategy::DouBaoStrategy() {
    const auto& apiKeys = AIConfig::getInstance().getApiKeysConfig();
    const auto& modelConfig = AIConfig::getInstance().getModelConfig();
    
    if (!apiKeys.doubaoApiKey.empty()) {
        apiKey_ = apiKeys.doubaoApiKey;
    } else {
        const char* key = std::getenv("DOUBAO_API_KEY");
        if (!key) throw std::runtime_error("DOUBAO API Key not found!");
        apiKey_ = std::string(key);
    }
    
    // 如果配置中有指定的模型ID，则使用配置中的，否则使用默认的
    if (!apiKeys.doubaoModelId.empty()) {
        modelName_ = apiKeys.doubaoModelId;
    } else {
        modelName_ = modelConfig.doubao.modelName;
    }
    
    apiUrl_ = modelConfig.doubao.apiUrl;
    isMCPModel = false;
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

// AliyunRAGStrategy implementation
AliyunRAGStrategy::AliyunRAGStrategy() {
    const auto& apiKeys = AIConfig::getInstance().getApiKeysConfig();
    const auto& modelConfig = AIConfig::getInstance().getModelConfig();
    
    if (!apiKeys.dashscopeApiKey.empty()) {
        apiKey_ = apiKeys.dashscopeApiKey;
    } else {
        const char* key = std::getenv("DASHSCOPE_API_KEY");
        if (!key) throw std::runtime_error("Aliyun API Key not found!");
        apiKey_ = std::string(key);
    }
    
    if (!apiKeys.knowledgeBaseId.empty()) {
        knowledgeBaseId_ = apiKeys.knowledgeBaseId;
    } else {
        const char* key = std::getenv("Knowledge_Base_ID");
        if (!key) throw std::runtime_error("Knowledge_Base_ID not found!");
        knowledgeBaseId_ = std::string(key);
    }
    
    apiUrlPrefix_ = modelConfig.aliyunRag.apiUrlPrefix;
    apiUrlSuffix_ = modelConfig.aliyunRag.apiUrlSuffix;
    isMCPModel = false;
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

// AliyunMcpStrategy implementation
AliyunMcpStrategy::AliyunMcpStrategy() {
    const auto& apiKeys = AIConfig::getInstance().getApiKeysConfig();
    const auto& modelConfig = AIConfig::getInstance().getModelConfig();
    
    if (!apiKeys.dashscopeApiKey.empty()) {
        apiKey_ = apiKeys.dashscopeApiKey;
    } else {
        const char* key = std::getenv("DASHSCOPE_API_KEY");
        if (!key) throw std::runtime_error("Aliyun API Key not found!");
        apiKey_ = std::string(key);
    }
    
    apiUrl_ = modelConfig.aliyunMcp.apiUrl;
    modelName_ = modelConfig.aliyunMcp.modelName;
    isMCPModel = true;
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