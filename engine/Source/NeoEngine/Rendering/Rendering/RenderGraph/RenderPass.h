#pragma once
#include <vector>
#include <functional>
#include <string>

namespace Neo {
    class RenderPass {
    public:
        std::string name;
        [[maybe_unused]] std::vector<std::string> reads;  // Resource Input
        [[maybe_unused]] std::vector<std::string> writes; // Resource Output
        std::function<void()> execute;

        RenderPass(const std::string& n, std::function<void()> fn)
            : name(n), execute(fn) {}
    };
}
