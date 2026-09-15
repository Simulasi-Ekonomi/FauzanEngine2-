#pragma once

#include <cstdint>

namespace NeoEngine {

enum class RuntimeFrameStage : uint8_t {
    InputSnapshot = 0,
    Simulation = 1,
    SceneSnapshot = 2,
    RenderCommands = 3,
    AudioEvents = 4,
    Completed = 5,
    Failed = 255
};

struct RuntimeFrameToken {
    uint64_t frame = 0U;
    uint64_t revision = 0U;
};

class RuntimeFrameContract final {
public:
    bool Begin(uint64_t frame, uint64_t revision) {
        if (m_Active) return false;
        if (frame == 0U && revision != 0U) return false;
        m_Token = {frame, revision};
        m_Stage = RuntimeFrameStage::InputSnapshot;
        m_Active = true;
        return true;
    }

    bool Advance(RuntimeFrameStage next) {
        if (!m_Active || next == RuntimeFrameStage::Failed) return false;
        const auto current = static_cast<uint8_t>(m_Stage);
        const auto requested = static_cast<uint8_t>(next);
        if (requested != current + 1U) return false;
        m_Stage = next;
        if (next == RuntimeFrameStage::Completed) m_Active = false;
        return true;
    }

    void Fail() {
        m_Stage = RuntimeFrameStage::Failed;
        m_Active = false;
    }

    bool IsActive() const { return m_Active; }
    bool IsComplete() const { return !m_Active && m_Stage == RuntimeFrameStage::Completed; }
    RuntimeFrameStage Stage() const { return m_Stage; }
    RuntimeFrameToken Token() const { return m_Token; }

private:
    RuntimeFrameToken m_Token{};
    RuntimeFrameStage m_Stage = RuntimeFrameStage::Failed;
    bool m_Active = false;
};

} // namespace NeoEngine
