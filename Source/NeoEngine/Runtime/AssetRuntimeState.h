#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace NeoEngine {

enum class AssetRuntimeState : uint8_t { Unregistered, Staged, Loading, Ready, Failed, Evicted };

struct AssetRuntimeRecord final {
    uint64_t contentHash = 0U;
    uint64_t generation = 0U;
    uint32_t residentBytes = 0U;
    AssetRuntimeState state = AssetRuntimeState::Unregistered;
};

class AssetRuntimeStateStore final {
public:
    bool Register(std::string_view id, uint64_t contentHash) {
        if (id.empty() || contentHash == 0U) return false;
        auto [it, inserted] = m_Records.emplace(std::string(id), AssetRuntimeRecord{contentHash, 1U, 0U, AssetRuntimeState::Unregistered});
        if (!inserted) return false;
        return true;
    }

    bool Transition(std::string_view id, AssetRuntimeState next, uint32_t residentBytes = 0U) {
        auto it = m_Records.find(std::string(id));
        if (it == m_Records.end()) return false;
        AssetRuntimeRecord& record = it->second;
        if (!IsLegal(record.state, next)) return false;
        record.state = next;
        if (next == AssetRuntimeState::Ready) record.residentBytes = residentBytes;
        if (next == AssetRuntimeState::Evicted) record.residentBytes = 0U;
        ++record.generation;
        return true;
    }

    const AssetRuntimeRecord* Find(std::string_view id) const {
        auto it = m_Records.find(std::string(id));
        return it == m_Records.end() ? nullptr : &it->second;
    }

    size_t Size() const { return m_Records.size(); }

private:
    static bool IsLegal(AssetRuntimeState from, AssetRuntimeState to) {
        switch (from) {
        case AssetRuntimeState::Unregistered: return to == AssetRuntimeState::Staged || to == AssetRuntimeState::Failed;
        case AssetRuntimeState::Staged: return to == AssetRuntimeState::Loading || to == AssetRuntimeState::Failed;
        case AssetRuntimeState::Loading: return to == AssetRuntimeState::Ready || to == AssetRuntimeState::Failed;
        case AssetRuntimeState::Ready: return to == AssetRuntimeState::Evicted || to == AssetRuntimeState::Loading;
        case AssetRuntimeState::Failed: return to == AssetRuntimeState::Staged || to == AssetRuntimeState::Loading;
        case AssetRuntimeState::Evicted: return to == AssetRuntimeState::Loading || to == AssetRuntimeState::Staged;
        }
        return false;
    }

    std::unordered_map<std::string, AssetRuntimeRecord> m_Records;
};

} // namespace NeoEngine
