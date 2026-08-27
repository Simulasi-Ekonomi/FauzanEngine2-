#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace NeoEngine {

enum class NetworkDelivery : uint8_t { Unreliable, ReliableOrdered };

struct NetworkDatagram {
    uint32_t peerId{};
    uint64_t sequence{};
    NetworkDelivery delivery{NetworkDelivery::Unreliable};
    std::vector<uint8_t> payload;
};

struct NetworkTransportStats {
    uint64_t sent{};
    uint64_t delivered{};
    uint64_t dropped{};
    uint64_t duplicated{};
    uint64_t reordered{};
    uint64_t rejected{};
};

// Transport-neutral packet scheduler. Real sockets are supplied by a platform adapter;
// this layer owns sequencing, bounded queues, and deterministic fault injection for tests.
class NetworkTransportQueue {
public:
    static constexpr uint16_t kCapacity = 1024;

    bool Enqueue(uint32_t peerId, NetworkDelivery delivery, std::span<const uint8_t> payload) {
        if (peerId == 0U || payload.empty() || count_ >= kCapacity) return false;
        NetworkDatagram& packet = packets_[(head_ + count_) % kCapacity];
        packet = {};
        packet.peerId = peerId;
        packet.sequence = ++sequence_;
        packet.delivery = delivery;
        packet.payload.assign(payload.begin(), payload.end());
        ++count_;
        ++stats_.sent;
        return true;
    }

    bool Dequeue(NetworkDatagram& packet) {
        if (count_ == 0U) return false;
        packet = std::move(packets_[head_]);
        packets_[head_] = {};
        head_ = (head_ + 1U) % kCapacity;
        --count_;
        ++stats_.delivered;
        return true;
    }

    bool DropNewest() {
        if (count_ == 0U) return false;
        const uint16_t index = static_cast<uint16_t>((head_ + count_ - 1U) % kCapacity);
        packets_[index] = {};
        --count_;
        ++stats_.dropped;
        return true;
    }

    bool ReorderTailPair() {
        if (count_ < 2U) return false;
        const uint16_t a = static_cast<uint16_t>((head_ + count_ - 1U) % kCapacity);
        const uint16_t b = static_cast<uint16_t>((head_ + count_ - 2U) % kCapacity);
        std::swap(packets_[a], packets_[b]);
        ++stats_.reordered;
        return true;
    }

    bool DuplicateTail() {
        if (count_ == 0U || count_ >= kCapacity) return false;
        const uint16_t source = static_cast<uint16_t>((head_ + count_ - 1U) % kCapacity);
        const uint16_t target = static_cast<uint16_t>((head_ + count_) % kCapacity);
        packets_[target] = packets_[source];
        ++count_;
        ++stats_.duplicated;
        return true;
    }

    [[nodiscard]] uint16_t Size() const { return count_; }
    [[nodiscard]] const NetworkTransportStats& Stats() const { return stats_; }

private:
    std::array<NetworkDatagram, kCapacity> packets_{};
    uint16_t head_{};
    uint16_t count_{};
    uint64_t sequence_{};
    NetworkTransportStats stats_{};
};

struct NetworkInterestVolume {
    float centerX{};
    float centerY{};
    float centerZ{};
    float radius{100.0F};
};

struct NetworkInterestEntry {
    uint32_t networkId{};
    bool relevant{};
};

class NetworkInterestFilter {
public:
    static bool IsRelevant(const NetworkInterestVolume& volume, float x, float y, float z) {
        if (!(volume.radius > 0.0F)) return false;
        const float dx = x - volume.centerX;
        const float dy = y - volume.centerY;
        const float dz = z - volume.centerZ;
        return dx * dx + dy * dy + dz * dz <= volume.radius * volume.radius;
    }
};

} // namespace NeoEngine
