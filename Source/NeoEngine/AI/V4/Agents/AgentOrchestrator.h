#pragma once
#include <vector>
#include <string>
#include <functional>
#include <cstdint>

namespace NeoEngine {

struct AgentCommand {
    uint32_t agentID;
    std::string action;
    float priority = 0.0f;
};

class AgentOrchestrator {
public:
    AgentOrchestrator();
    ~AgentOrchestrator();

    void AddAgent(uint32_t id, const std::string& role);
    void RemoveAgent(uint32_t id);
    void SubmitCommand(const std::string& role, const AgentCommand& cmd);
    void ProcessCommands(std::function<void(uint32_t, const std::string&)> executor);
    size_t GetAgentCount() const;

private:
    struct AgentInfo {
        uint32_t id;
        std::string role;
    };
    std::vector<AgentInfo> agents_;
    std::vector<AgentCommand> pendingCommands_;
};

} // namespace
