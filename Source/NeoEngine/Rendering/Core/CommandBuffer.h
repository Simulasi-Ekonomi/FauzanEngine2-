#pragma once
#include <vector>
#include <functional>

namespace NeoEngine {

class CommandBuffer {
public:
    void Begin();
    void End();
    void Clear(float r, float g, float b, float a);
    void DrawIndexedInstanced(uint32_t instanceCount);
    // Nanti bisa ditambah set shader, set constants, dll.
    void Execute();
private:
    struct Command {
        enum Type { CLEAR, DRAW_INDEXED_INSTANCED } type;
        float clearColor[4];
        uint32_t instanceCount;
    };
    std::vector<Command> commands_;
};

inline void CommandBuffer::Begin() {
    commands_.clear();
}

inline void CommandBuffer::End() {}

inline void CommandBuffer::Clear(float r, float g, float b, float a) {
    commands_.push_back({Command::CLEAR, {r, g, b, a}, 0});
}

inline void CommandBuffer::DrawIndexedInstanced(uint32_t instanceCount) {
    commands_.push_back({Command::DRAW_INDEXED_INSTANCED, {}, instanceCount});
}

inline void CommandBuffer::Execute() {
    for (auto& cmd : commands_) {
        if (cmd.type == Command::CLEAR) {
            // Panggil RHI clear
        } else if (cmd.type == Command::DRAW_INDEXED_INSTANCED) {
            // Panggil RHI draw
        }
    }
}

} // namespace
