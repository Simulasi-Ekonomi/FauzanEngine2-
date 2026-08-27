#pragma once
#include <cstdint>
#include <cstddef>

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
        sessionId_ = record.sessionId;
        revision_ = record.revision;
        data_ = record.data;
        size_ = record.size;
        hasRevision_ = true;
        return true;
    }

    bool commit(uint64_t sessionId, uint64_t revision) {
        return hasRevision_ && sessionId == sessionId_ && revision == revision_;
    }

    bool load(uint64_t sessionId, uint64_t revision, DurableStateRecord& out) const {
        if (!hasRevision_ || sessionId != sessionId_ || revision != revision_) return false;
        out = {sessionId_, revision_, data_, size_};
        return true;
    }

    void clear() { sessionId_ = revision_ = size_ = 0; data_ = nullptr; hasRevision_ = false; }
    uint64_t revision() const { return revision_; }
    size_t size() const { return size_; }

private:
    uint64_t sessionId_{};
    uint64_t revision_{};
    const std::byte* data_{nullptr};
    size_t size_{};
    bool hasRevision_{false};
};

} // namespace NeoEngine::Networking
