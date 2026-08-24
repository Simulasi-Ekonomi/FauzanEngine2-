#pragma once
#include <vector>
#include <functional>

namespace Neo {
    struct GPUCommand {
        std::function<void()> record;
    };

    class CommandBuffer {
        [[maybe_unused]] std::vector<GPUCommand> cmds;
    public:
        void add(const GPUCommand& c) { cmds.push_back(c); }
        void submit() {
            for(auto& c : cmds) c.record();
        }
        void clear() { cmds.clear(); }
    };
}
