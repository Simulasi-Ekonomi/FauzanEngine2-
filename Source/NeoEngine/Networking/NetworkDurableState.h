#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace NeoEngine::Networking {

struct DurableStateRecord {
    uint64_t sessionId{};
    uint64_t revision{};
    const std::byte* data{nullptr};
    size_t size{0};
};

class DurableStateStore {
public:
    static constexpr size_t MaxStateBytes = 4 * 1024 * 1024;

    bool stage(const DurableStateRecord& record) {
        if (!record.sessionId || !record.revision || !record.data || !record.size || record.size > MaxStateBytes) return false;
        if (hasRevision_ && record.revision <= revision_) return false;
        ownedData_.assign(record.data, record.data + record.size);
        sessionId_ = record.sessionId;
        revision_ = record.revision;
        hasRevision_ = true;
        return true;
    }

    bool commit(uint64_t sessionId, uint64_t revision) const {
        return hasRevision_ && sessionId == sessionId_ && revision == revision_;
    }

    bool load(uint64_t sessionId, uint64_t revision, DurableStateRecord& out) const {
        if (!hasRevision_ || sessionId != sessionId_ || revision != revision_) return false;
        out = {sessionId_, revision_, ownedData_.data(), ownedData_.size()};
        return true;
    }

    void clear() {
        ownedData_.clear();
        ownedData_.shrink_to_fit();
        sessionId_ = revision_ = 0;
        hasRevision_ = false;
    }

    uint64_t revision() const { return revision_; }
    size_t size() const { return ownedData_.size(); }

private:
    uint64_t sessionId_{};
    uint64_t revision_{};
    std::vector<std::byte> ownedData_{};
    bool hasRevision_{false};
};

} // namespace NeoEngine::Networking
