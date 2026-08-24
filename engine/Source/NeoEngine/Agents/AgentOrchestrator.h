#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>

namespace NeoEngine {

enum class AgentType { Director, Artist, Developer, Tester, Analyst, Security };

struct AgentTask {
    std::string id;
    std::string description;
    AgentType assignedTo;
    std::string status; // "pending", "running", "completed", "failed"
    std::string result;
};

class AgentOrchestrator {
public:
    static AgentOrchestrator& Get() {
        static AgentOrchestrator instance;
        return instance;
    }

    std::string AssignTask(const std::string& description, AgentType type) {
        std::string id = "task_" + std::to_string(m_NextTaskId++);
        AgentTask task{id, description, type, "pending", ""};
        m_Tasks[id] = task;
        ProcessTask(id);
        return id;
    }

    std::string GetTaskStatus(const std::string& taskId) {
        auto it = m_Tasks.find(taskId);
        return it != m_Tasks.end() ? it->second.status : "not_found";
    }

    std::string GetTaskResult(const std::string& taskId) {
        auto it = m_Tasks.find(taskId);
        return it != m_Tasks.end() ? it->second.result : "";
    }

    void RegisterAgent(AgentType type, std::function<std::string(const std::string&)> handler) {
        m_AgentHandlers[type] = handler;
    }

    std::string GetStatusReport() const {
        return "Active tasks: " + std::to_string(m_Tasks.size()) +
               ", Registered agents: " + std::to_string(m_AgentHandlers.size());
    }

private:
    AgentOrchestrator() = default;

    void ProcessTask(const std::string& taskId) {
        auto it = m_Tasks.find(taskId);
        if (it == m_Tasks.end()) return;
        auto& task = it->second;
        auto handler = m_AgentHandlers.find(task.assignedTo);
        if (handler != m_AgentHandlers.end()) {
            task.status = "running";
            task.result = handler->second(task.description);
            task.status = "completed";
        } else {
            task.status = "failed";
            task.result = "No agent handler for " + std::to_string((int)task.assignedTo);
        }
    }

    std::unordered_map<std::string, AgentTask> m_Tasks;
    std::unordered_map<AgentType, std::function<std::string(const std::string&)>> m_AgentHandlers;
    int m_NextTaskId = 1;
};

} // namespace NeoEngine
