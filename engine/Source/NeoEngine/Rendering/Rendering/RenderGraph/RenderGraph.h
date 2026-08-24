#pragma once
#include "RenderPass.h"
#include <vector>

namespace Neo {
    class RenderGraph {
        [[maybe_unused]] std::vector<RenderPass> passes;
    public:
        void add(const RenderPass& p);
        void execute();
    };
}
