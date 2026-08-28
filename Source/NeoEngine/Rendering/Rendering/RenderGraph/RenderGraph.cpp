#include "RenderGraph.h"

namespace NeoEngine {
bool RenderGraph::compile() {
    const std::size_t count = passes.size();
    std::vector<std::vector<std::size_t>> edges(count);
    std::vector<std::size_t> indegree(count, 0);
    std::unordered_map<std::string, std::size_t> producer;
    for (std::size_t i = 0; i < count; ++i) {
        for (const auto& resource : passes[i].writes) {
            if (!resource.empty()) producer[resource] = i;
        }
    }
    for (std::size_t i = 0; i < count; ++i) {
        for (const auto& resource : passes[i].reads) {
            const auto it = producer.find(resource);
            if (it == producer.end() || it->second == i) continue;
            const std::size_t source = it->second;
            if (std::find(edges[source].begin(), edges[source].end(), i) == edges[source].end()) {
                edges[source].push_back(i);
                ++indegree[i];
            }
        }
    }
    std::vector<std::size_t> ready;
    for (std::size_t i = 0; i < count; ++i) if (indegree[i] == 0) ready.push_back(i);
    std::vector<std::size_t> order;
    while (!ready.empty()) {
        const std::size_t current = ready.front();
        ready.erase(ready.begin());
        order.push_back(current);
        for (const std::size_t target : edges[current]) {
            if (--indegree[target] == 0) {
                ready.push_back(target);
                std::sort(ready.begin(), ready.end());
            }
        }
    }
    if (order.size() != count) {
        executionOrder_.clear();
        return false;
    }
    executionOrder_.clear();
    for (const std::size_t index : order) executionOrder_.push_back(passes[index].name);
    return true;
}

bool RenderGraph::executeChecked() {
    if (!compile()) return false;
    for (const auto& name : executionOrder_) {
        for (auto& pass : passes) {
            if (pass.name == name && pass.execute) {
                pass.execute();
                break;
            }
        }
    }
    return true;
}
} // namespace NeoEngine
