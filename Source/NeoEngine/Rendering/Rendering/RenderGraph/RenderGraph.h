#pragma once
#include "RenderPass.h"
#include <algorithm>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace NeoEngine {
class RenderGraph {
public:
    void add(const RenderPass& p) { passes.push_back(p); }
    bool compile();
    bool executeChecked();
    void execute() { (void)executeChecked(); }
    [[nodiscard]] const std::vector<std::string>& executionOrder() const { return executionOrder_; }
    void clear() { passes.clear(); executionOrder_.clear(); }
private:
    std::vector<RenderPass> passes;
    std::vector<std::string> executionOrder_;
};
}
