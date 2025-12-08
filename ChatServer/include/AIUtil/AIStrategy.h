#pragma once

#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include <sstream>
#include <memory>

#include "utils/JsonUtil.h"
#include "AIUtil/AIConfig.h"

class AIStrategy {
public:
    virtual ~AIStrategy() = default;

    virtual std::string getApiUrl() const = 0;
    
    virtual std::string getApiKey() const = 0;

    virtual std::string getModel() const = 0;

    virtual json buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const = 0;

    virtual std::string parseResponse(const json& response) const = 0;

    bool isMCPModel = false;
};

class AliyunStrategy : public AIStrategy {
public:
    AliyunStrategy() {
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

    std::string getApiUrl() const override;
    std::string getApiKey() const override;
    std::string getModel() const override;

    json buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const override;
    std::string parseResponse(const json& response) const override;

private:
    std::string apiKey_;
    std::string apiUrl_;
    std::string modelName_;
};

class DouBaoStrategy : public AIStrategy {
public:
    DouBaoStrategy() {
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

    std::string getApiUrl() const override;
    std::string getApiKey() const override;
    std::string getModel() const override;

    json buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const override;
    std::string parseResponse(const json& response) const override;

private:
    std::string apiKey_;
    std::string apiUrl_;
    std::string modelName_;
};

class AliyunRAGStrategy : public AIStrategy {
public:
    AliyunRAGStrategy() {
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

    std::string getApiUrl() const override;
    std::string getApiKey() const override;
    std::string getModel() const override;

    json buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const override;
    std::string parseResponse(const json& response) const override;

private:
    std::string apiKey_;
    std::string knowledgeBaseId_;
    std::string apiUrlPrefix_;
    std::string apiUrlSuffix_;
};

class AliyunMcpStrategy : public AIStrategy {
public:
    AliyunMcpStrategy() {
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

    std::string getApiUrl() const override;
    std::string getApiKey() const override;
    std::string getModel() const override;

    json buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const override;
    std::string parseResponse(const json& response) const override;

private:
    std::string apiKey_;
    std::string apiUrl_;
    std::string modelName_;
};