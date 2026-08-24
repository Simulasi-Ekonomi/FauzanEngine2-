#pragma once
#include "FrameGraphPass.h"
#include <vector>
#include <unordered_map>

namespace Neo {
    class FrameGraph {
        [[maybe_unused]] std::vector<FrameGraphPass> passes;
    public:
        void addPass(const FrameGraphPass& pass) {
            passes.push_back(pass);
        }

        // TODO: Implementasi Topological Sort di masa depan untuk otomatisasi dependency
        void execute() {
            for(auto& p : passes) {
                // Di sini nanti tempat integrasi Profiler & GPU Marker
                p.execute();
            }
        }

        void clear() {
            passes.clear();
        }
    };
}
