#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace NeoEngine {

class CommandIdempotency final {
public:
    explicit CommandIdempotency(size_t capacity = 4096U) : m_Capacity(capacity) {}

    bool Accept(uint64_t commandId, uint64_t payloadHash) {
        if (commandId == 0U || payloadHash == 0U || m_Capacity == 0U) return false;
        auto it = m_Seen.find(commandId);
        if (it != m_Seen.end()) return it->second == payloadHash;
        if (m_Seen.size() >= m_Capacity) return false;
        m_Seen.emplace(commandId, payloadHash);
        return true;
    }

    bool Matches(uint64_t commandId, uint64_t payloadHash) const {
        auto it = m_Seen.find(commandId);
        return it != m_Seen.end() && it->second == payloadHash;
    }

    size_t Size() const { return m_Seen.size(); }
    void Clear() { m_Seen.clear(); }

private:
    size_t m_Capacity;
    std::unordered_map<uint64_t, uint64_t> m_Seen;
};

} // namespace NeoEngine
