#pragma once
#include <array>
#include <cstdint>

namespace NeoEngine::Networking {

struct NetworkCommandKey {
    uint32_t peerId{};
    uint64_t sequence{};
    uint32_t commandType{};
};

class NetworkCommandGuard {
public:
    static constexpr uint16_t Capacity = 1024;

    bool accept(const NetworkCommandKey& key) {
        if (key.peerId == 0U || key.sequence == 0U || key.commandType == 0U) return false;
        if (contains(key)) return false;
        if (count_ >= Capacity) evictOldest();
        entries_[(head_ + count_) % Capacity] = key;
        ++count_;
        return true;
    }

    bool contains(const NetworkCommandKey& key) const {
        for (uint16_t i = 0; i < count_; ++i) {
            const auto& candidate = entries_[(head_ + i) % Capacity];
            if (candidate.peerId == key.peerId &&
                candidate.sequence == key.sequence &&
                candidate.commandType == key.commandType) return true;
        }
        return false;
    }

    uint16_t discardThrough(uint32_t peerId, uint64_t sequence) {
        uint16_t removed = 0;
        while (count_ != 0U) {
            const auto& oldest = entries_[head_];
            if (oldest.peerId != peerId || oldest.sequence > sequence) break;
            head_ = static_cast<uint16_t>((head_ + 1U) % Capacity);
            --count_;
            ++removed;
        }
        return removed;
    }

    [[nodiscard]] uint16_t size() const { return count_; }

private:
    void evictOldest() {
        head_ = static_cast<uint16_t>((head_ + 1U) % Capacity);
        --count_;
    }

    std::array<NetworkCommandKey, Capacity> entries_{};
    uint16_t head_{};
    uint16_t count_{};
};

} // namespace NeoEngine::Networking
