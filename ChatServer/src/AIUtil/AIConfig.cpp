#include "AIUtil/AIConfig.h"

AIConfig::AIConfig() : isLoaded_(false) {
    // 初始化默认值
    dbConfig_.host = "tcp://127.0.0.1:3306";
    dbConfig_.user = "root";
    dbConfig_.password = "root";
    dbConfig_.database = "ChatHttpServer";
    dbConfig_.poolSize = 5;
    
    mqConfig_.host = "localhost";
    mqConfig_.port = 5672;
    mqConfig_.username = "guest";
    mqConfig_.password = "guest";
    mqConfig_.vhost = "/";
    mqConfig_.queueName = "sql_queue";
    mqConfig_.threadNum = 2;
    
    // API密钥默认为空
    apiKeysConfig_.dashscopeApiKey = "";
    apiKeysConfig_.knowledgeBaseId = "";
    apiKeysConfig_.baiduClientId = "";
    apiKeysConfig_.baiduClientSecret = "";
    apiKeysConfig_.doubaoApiKey = "";
    apiKeysConfig_.doubaoModelId = "";
    
    // 模型配置默认值
    modelConfig_.aliyun.apiUrl = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions";
    modelConfig_.aliyun.modelName = "qwen-plus";
    modelConfig_.doubao.apiUrl = "https://ark.cn-beijing.volces.com/api/v3/chat/completions";
    modelConfig_.doubao.modelName = "doubao-seed-1-6-thinking-250715";
    modelConfig_.aliyunRag.apiUrlPrefix = "https://dashscope.aliyuncs.com/api/v1/apps/";
    modelConfig_.aliyunRag.apiUrlSuffix = "/completion";
    modelConfig_.aliyunMcp.apiUrl = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions";
    modelConfig_.aliyunMcp.modelName = "qwen-plus";
}

AIConfig& AIConfig::getInstance() {
    static AIConfig instance;
    return instance;
}

bool AIConfig::loadFromFile(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << path << std::endl;
            return false;
        }

        json config;
        file >> config;
        file.close();

        // 加载提示模板
        if (config.contains("prompt_template")) {
            promptTemplate_ = config["prompt_template"];
        }

        // 加载数据库配置
        if (config.contains("database")) {
            auto dbConfig = config["database"];
            if (dbConfig.contains("host")) {
                dbConfig_.host = dbConfig["host"];
            }
            if (dbConfig.contains("user")) {
                dbConfig_.user = dbConfig["user"];
            }
            if (dbConfig.contains("password")) {
                dbConfig_.password = dbConfig["password"];
            }
            if (dbConfig.contains("database")) {
                dbConfig_.database = dbConfig["database"];
            }
            if (dbConfig.contains("pool_size")) {
                dbConfig_.poolSize = dbConfig["pool_size"];
            }
        }

        // 加载RabbitMQ配置
        if (config.contains("rabbitmq")) {
            auto mqConfig = config["rabbitmq"];
            if (mqConfig.contains("host")) {
                mqConfig_.host = mqConfig["host"];
            }
            if (mqConfig.contains("port")) {
                mqConfig_.port = mqConfig["port"];
            }
            if (mqConfig.contains("username")) {
                mqConfig_.username = mqConfig["username"];
            }
            if (mqConfig.contains("password")) {
                mqConfig_.password = mqConfig["password"];
            }
            if (mqConfig.contains("vhost")) {
                mqConfig_.vhost = mqConfig["vhost"];
            }
            if (mqConfig.contains("queue_name")) {
                mqConfig_.queueName = mqConfig["queue_name"];
            }
            if (mqConfig.contains("thread_num")) {
                mqConfig_.threadNum = mqConfig["thread_num"];
            }
        }

        // 加载API密钥配置
        if (config.contains("api_keys")) {
            auto apiKeysConfig = config["api_keys"];
            if (apiKeysConfig.contains("dashscope_api_key")) {
                apiKeysConfig_.dashscopeApiKey = apiKeysConfig["dashscope_api_key"];
            }
            if (apiKeysConfig.contains("knowledge_base_id")) {
                apiKeysConfig_.knowledgeBaseId = apiKeysConfig["knowledge_base_id"];
            }
            if (apiKeysConfig.contains("baidu_client_id")) {
                apiKeysConfig_.baiduClientId = apiKeysConfig["baidu_client_id"];
            }
            if (apiKeysConfig.contains("baidu_client_secret")) {
                apiKeysConfig_.baiduClientSecret = apiKeysConfig["baidu_client_secret"];
            }
            if (apiKeysConfig.contains("doubao_api_key")) {
                apiKeysConfig_.doubaoApiKey = apiKeysConfig["doubao_api_key"];
            }
        }

        // 加载模型配置
        if (config.contains("models")) {
            auto modelsConfig = config["models"];
            
            // 阿里云模型配置
            if (modelsConfig.contains("aliyun")) {
                auto aliyunConfig = modelsConfig["aliyun"];
                if (aliyunConfig.contains("api_url")) {
                    modelConfig_.aliyun.apiUrl = aliyunConfig["api_url"];
                }
                if (aliyunConfig.contains("model_name")) {
                    modelConfig_.aliyun.modelName = aliyunConfig["model_name"];
                }
            }
            
            // 豆包模型配置
            if (modelsConfig.contains("doubao")) {
                auto doubaoConfig = modelsConfig["doubao"];
                if (doubaoConfig.contains("api_url")) {
                    modelConfig_.doubao.apiUrl = doubaoConfig["api_url"];
                }
                if (doubaoConfig.contains("model_name")) {
                    modelConfig_.doubao.modelName = doubaoConfig["model_name"];
                }
            }
            
            // 阿里云RAG配置
            if (modelsConfig.contains("aliyun_rag")) {
                auto aliyunRagConfig = modelsConfig["aliyun_rag"];
                if (aliyunRagConfig.contains("api_url_prefix")) {
                    modelConfig_.aliyunRag.apiUrlPrefix = aliyunRagConfig["api_url_prefix"];
                }
                if (aliyunRagConfig.contains("api_url_suffix")) {
                    modelConfig_.aliyunRag.apiUrlSuffix = aliyunRagConfig["api_url_suffix"];
                }
            }
            
            // 阿里云MCP配置
            if (modelsConfig.contains("aliyun_mcp")) {
                auto aliyunMcpConfig = modelsConfig["aliyun_mcp"];
                if (aliyunMcpConfig.contains("api_url")) {
                    modelConfig_.aliyunMcp.apiUrl = aliyunMcpConfig["api_url"];
                }
                if (aliyunMcpConfig.contains("model_name")) {
                    modelConfig_.aliyunMcp.modelName = aliyunMcpConfig["model_name"];
                }
            }
        }

        // 加载工具配置
        if (config.contains("tools")) {
            tools_.clear();
            for (const auto& toolItem : config["tools"]) {
                AITool tool;
                if (toolItem.contains("name")) {
                    tool.name = toolItem["name"];
                }
                if (toolItem.contains("desc")) {
                    tool.desc = toolItem["desc"];
                }
                if (toolItem.contains("params")) {
                    for (auto it = toolItem["params"].begin(); it != toolItem["params"].end(); ++it) {
                        tool.params[it.key()] = it.value();
                    }
                }
                tools_.push_back(tool);
            }
        }

        isLoaded_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load config from " << path << ": " << e.what() << std::endl;
        return false;
    }
}

std::string AIConfig::buildToolList() const {
    // 优先使用工具注册中心的工具列表
    std::string registryTools = toolRegistry_.getToolListDescription();
    if (!registryTools.empty()) {
        return registryTools;
    }
    
    // 如果注册中心没有工具，则使用配置文件中的工具列表
    std::ostringstream oss;
    for (const auto& t : tools_) {
        oss << t.name << "(";
        bool first = true;
        for (const auto& [key, val] : t.params) {
            if (!first) oss << ", ";
            oss << key;
            first = false;
        }
        oss << ") -> " << t.desc << "\n";
    }
    return oss.str();
}

std::string AIConfig::buildPrompt(const std::string& userInput) const {
    if (!isLoaded_) {
        return ""; // 配置未加载
    }
    
    std::string result = promptTemplate_;
    result = std::regex_replace(result, std::regex("\\{user_input\\}"), userInput);
    result = std::regex_replace(result, std::regex("\\{tool_list\\}"), buildToolList());
    return result;
}

AIToolCall AIConfig::parseAIResponse(const std::string& response) const {
    AIToolCall result;
    try {
        json j = json::parse(response);

        if (j.contains("tool") && j["tool"].is_string()) {
            result.toolName = j["tool"].get<std::string>();
            if (j.contains("args") && j["args"].is_object()) {
                result.args = j["args"];
            }
            result.isToolCall = true;
        }
    }
    catch (...) {
        result.isToolCall = false;
    }
    return result;
}

std::string AIConfig::buildToolResultPrompt(
    const std::string& userInput,
    const std::string& toolName,
    const json& toolArgs,
    const json& toolResult) const
{
    std::ostringstream oss;
    oss << "下面是用户说的话：" << userInput << "\n"
        << "我刚才调用了工具 [" << toolName << "] ，参数为："
        << toolArgs.dump() << "\n"
        << "工具返回的结果如下：\n" << toolResult.dump(4) << "\n"
        << "请根据以上信息，用自然语言回答用户。";
    return oss.str();
}