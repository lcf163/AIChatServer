#include "AIUtil/AIFactory.h"

StrategyFactory& StrategyFactory::instance() {
    static StrategyFactory factory;
    return factory;
}

void StrategyFactory::registerStrategy(const std::string& name, Creator creator) {
    creators_[name] = std::move(creator);
}

std::shared_ptr<AIStrategy> StrategyFactory::create(const std::string& name) {
    auto it = creators_.find(name);
    if (it == creators_.end()) {
        throw std::runtime_error("Unknown strategy: " + name);
    }
    return it->second();
}