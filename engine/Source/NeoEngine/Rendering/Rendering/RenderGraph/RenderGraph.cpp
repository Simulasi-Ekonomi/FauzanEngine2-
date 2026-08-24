#include <cassert>
#include "RenderGraph.h"

namespace Neo {
    void RenderGraph::add(const RenderPass& p) {
        passes.push_back(p);
    }

    void RenderGraph::execute() {
        // Simple linear execution for now
        // Next stage: Multi-threaded dispatch via DependencySolver
        for(auto& p : passes) {
            p.execute();
        }
    }
}
