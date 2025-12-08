#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <regex>
#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>

#include "utils/JsonUtil.h"  
#include "AIToolRegistry.h"

struct AITool {
    std::string name;
    std::unordered_map<std::string, std::string> params;
    std::string desc;
};

struct AIToolCall {
    std::string toolName;
    json args;
    bool isToolCall = false;
};

class AIConfig {
public:
    static AIConfig& getInstance();
    
    bool loadFromFile(const std::string& path);
    std::string buildPrompt(const std::string& userInput) const;
    AIToolCall parseAIResponse(const std::string& response) const;
    std::string buildToolResultPrompt(
        const std::string& userInput,
        const std::string& toolName,
        const json& toolArgs,
        const json& toolResult) const;

private:
    AIConfig(); // 私有构造函数
    ~AIConfig() = default;
    AIConfig(const AIConfig&) = delete; // 禁止拷贝构造
    AIConfig& operator=(const AIConfig&) = delete; // 禁止赋值操作
    
    std::string promptTemplate_;
    std::vector<AITool> tools_;
    mutable AIToolRegistry toolRegistry_; // 用于获取工具列表信息
    bool isLoaded_;

    std::string buildToolList() const;
};