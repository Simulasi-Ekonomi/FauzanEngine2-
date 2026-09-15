#pragma once

#include <cstdint>

namespace NeoEngine {

struct SecurityCommandPolicy final {
    uint32_t maxPayloadBytes = 64U * 1024U;
    uint32_t maxCommandsPerFrame = 256U;
    bool requireMonotonicSequence = true;
};

class SecurityCommandValidator final {
public:
    explicit SecurityCommandValidator(SecurityCommandPolicy policy = {}) : m_Policy(policy) {}

    bool Validate(uint64_t sequence, uint32_t payloadBytes) {
        if (sequence == 0U || payloadBytes > m_Policy.maxPayloadBytes) return false;
        if (m_Policy.requireMonotonicSequence && sequence <= m_LastSequence) return false;
        if (m_FrameCommands >= m_Policy.maxCommandsPerFrame) return false;
        m_LastSequence = sequence;
        ++m_FrameCommands;
        return true;
    }

    void BeginFrame() { m_FrameCommands = 0U; }
    uint64_t LastSequence() const { return m_LastSequence; }
    uint32_t FrameCommands() const { return m_FrameCommands; }

private:
    SecurityCommandPolicy m_Policy;
    uint64_t m_LastSequence = 0U;
    uint32_t m_FrameCommands = 0U;
};

} // namespace NeoEngine
