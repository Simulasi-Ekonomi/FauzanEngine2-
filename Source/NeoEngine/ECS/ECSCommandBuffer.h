#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace NeoEngine {

struct ECSCommand final {
    enum class Kind : uint8_t { CreateEntity, DestroyEntity, SetComponent, RemoveComponent };
    Kind kind{};
    uint32_t entity = 0U;
    uint32_t componentType = 0U;
    uint64_t payloadOffset = 0U;
    uint32_t payloadSize = 0U;
};

class ECSCommandBuffer final {
public:
    static constexpr uint32_t kMaxCommands = 65536U;
    static constexpr uint32_t kMaxPayloadBytes = 4U * 1024U * 1024U;

    bool CreateEntity(uint32_t entity) { return Push({ECSCommand::Kind::CreateEntity, entity, 0U, 0U, 0U}); }
    bool DestroyEntity(uint32_t entity) { return Push({ECSCommand::Kind::DestroyEntity, entity, 0U, 0U, 0U}); }

    bool SetComponent(uint32_t entity, uint32_t componentType, std::span<const uint8_t> payload) {
        if (payload.empty() || payload.size() > UINT32_MAX || m_Payload.size() > kMaxPayloadBytes - payload.size()) return false;
        const uint64_t offset = m_Payload.size();
        m_Payload.insert(m_Payload.end(), payload.begin(), payload.end());
        if (!Push({ECSCommand::Kind::SetComponent, entity, componentType, offset, static_cast<uint32_t>(payload.size())})) {
            m_Payload.resize(static_cast<size_t>(offset));
            return false;
        }
        return true;
    }

    bool RemoveComponent(uint32_t entity, uint32_t componentType) {
        return Push({ECSCommand::Kind::RemoveComponent, entity, componentType, 0U, 0U});
    }

    std::span<const ECSCommand> Commands() const { return m_Commands; }
    std::span<const uint8_t> Payload(const ECSCommand& command) const {
        if (command.payloadSize == 0U || command.payloadOffset > m_Payload.size() || command.payloadSize > m_Payload.size() - command.payloadOffset) return {};
        return {m_Payload.data() + command.payloadOffset, command.payloadSize};
    }
    void Clear() { m_Commands.clear(); m_Payload.clear(); }
    bool Empty() const { return m_Commands.empty(); }

private:
    bool Push(const ECSCommand& command) {
        if (m_Commands.size() >= kMaxCommands) return false;
        m_Commands.push_back(command);
        return true;
    }

    std::vector<ECSCommand> m_Commands;
    std::vector<uint8_t> m_Payload;
};

} // namespace NeoEngine
