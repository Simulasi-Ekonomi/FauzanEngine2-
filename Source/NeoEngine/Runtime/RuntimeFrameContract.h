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

struct RuntimeFrameReceipt {
    RuntimeFrameToken token{};
    RuntimeFrameStage completedStage = RuntimeFrameStage::Failed;
    uint64_t inputDigest = 0U;
    uint64_t simulationDigest = 0U;
    uint64_t sceneDigest = 0U;
    uint64_t renderDigest = 0U;
    uint64_t audioDigest = 0U;
    bool completed = false;
};

class RuntimeFrameContract final {
public:
    bool Begin(uint64_t frame, uint64_t revision) {
        if (m_Active) return false;
        if (frame == 0U && revision != 0U) return false;
        m_Token = {frame, revision};
        m_Receipt = {};
        m_Receipt.token = m_Token;
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
        if (next == RuntimeFrameStage::Completed) { m_Receipt.completed = true; m_Receipt.completedStage = next; m_Active = false; }
        return true;
    }

    bool CommitStageDigest(RuntimeFrameStage stage, uint64_t digest) {
        if (!m_Active || stage != m_Stage || digest == 0U) return false;
        switch (stage) {
            case RuntimeFrameStage::InputSnapshot: if (m_Receipt.inputDigest != 0U) return false; m_Receipt.inputDigest = digest; break;
            case RuntimeFrameStage::Simulation: if (m_Receipt.simulationDigest != 0U) return false; m_Receipt.simulationDigest = digest; break;
            case RuntimeFrameStage::SceneSnapshot: if (m_Receipt.sceneDigest != 0U) return false; m_Receipt.sceneDigest = digest; break;
            case RuntimeFrameStage::RenderCommands: if (m_Receipt.renderDigest != 0U) return false; m_Receipt.renderDigest = digest; break;
            case RuntimeFrameStage::AudioEvents: if (m_Receipt.audioDigest != 0U) return false; m_Receipt.audioDigest = digest; break;
            case RuntimeFrameStage::Completed: break;
            case RuntimeFrameStage::Failed: return false;
        }
        return true;
    }

    void Fail() {
        m_Stage = RuntimeFrameStage::Failed;
        m_Active = false;
        m_Receipt.completed = false;
        m_Receipt.completedStage = RuntimeFrameStage::Failed;
    }

    [[nodiscard]] RuntimeFrameReceipt Receipt() const { return m_Receipt; }

    bool IsActive() const { return m_Active; }
    bool IsComplete() const { return !m_Active && m_Stage == RuntimeFrameStage::Completed; }
    RuntimeFrameStage Stage() const { return m_Stage; }
    RuntimeFrameToken Token() const { return m_Token; }

private:
    RuntimeFrameToken m_Token{};
    RuntimeFrameReceipt m_Receipt{};
    RuntimeFrameStage m_Stage = RuntimeFrameStage::Failed;
    bool m_Active = false;
};

} // namespace NeoEngine
