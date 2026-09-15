#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace NeoEngine {

struct RenderPass final {
    uint32_t id = 0U;
    std::string name;
    std::vector<uint32_t> dependencies;
};

class RenderFrameGraph final {
public:
    bool AddPass(uint32_t id, std::string name, std::vector<uint32_t> dependencies = {}) {
        if (id == 0U || name.empty() || m_Passes.contains(id)) return false;
        for (uint32_t dependency : dependencies) {
            if (dependency == 0U || dependency == id) return false;
        }
        m_Passes.emplace(id, RenderPass{id, std::move(name), std::move(dependencies)});
        return !HasCycle();
    }

    bool RemovePass(uint32_t id) {
        if (!m_Passes.contains(id)) return false;
        for (const auto& [_, pass] : m_Passes) {
            for (uint32_t dependency : pass.dependencies) if (dependency == id) return false;
        }
        m_Passes.erase(id);
        return true;
    }

    bool BuildOrder(std::vector<uint32_t>& order) const {
        order.clear();
        std::unordered_map<uint32_t, uint32_t> indegree;
        for (const auto& [id, _] : m_Passes) indegree[id] = 0U;
        for (const auto& [_, pass] : m_Passes) {
            for (uint32_t dependency : pass.dependencies) {
                if (!m_Passes.contains(dependency)) return false;
                ++indegree[pass.id];
            }
        }
        std::vector<uint32_t> ready;
        for (const auto& [id, degree] : indegree) if (degree == 0U) ready.push_back(id);
        while (!ready.empty()) {
            const uint32_t id = ready.back();
            ready.pop_back();
            order.push_back(id);
            for (const auto& [passId, pass] : m_Passes) {
                for (uint32_t dependency : pass.dependencies) {
                    if (dependency == id && --indegree[passId] == 0U) ready.push_back(passId);
                }
            }
        }
        return order.size() == m_Passes.size();
    }

    size_t Size() const { return m_Passes.size(); }

private:
    bool HasCycle() const {
        std::unordered_map<uint32_t, uint8_t> color;
        for (const auto& [id, _] : m_Passes) color[id] = 0U;
        for (const auto& [id, _] : m_Passes) if (color[id] == 0U && Visit(id, color)) return true;
        return false;
    }

    bool Visit(uint32_t id, std::unordered_map<uint32_t, uint8_t>& color) const {
        color[id] = 1U;
        const auto it = m_Passes.find(id);
        if (it == m_Passes.end()) return true;
        for (uint32_t dependency : it->second.dependencies) {
            if (!m_Passes.contains(dependency)) continue;
            if (color[dependency] == 1U) return true;
            if (color[dependency] == 0U && Visit(dependency, color)) return true;
        }
        color[id] = 2U;
        return false;
    }

    std::unordered_map<uint32_t, RenderPass> m_Passes;
};

} // namespace NeoEngine
