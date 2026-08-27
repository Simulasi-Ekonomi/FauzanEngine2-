#pragma once
#include <array>
#include <cstdint>

namespace NeoEngine::Networking {

enum class Delivery : uint8_t { Unreliable, ReliableOrdered };

struct Packet {
    uint64_t sequence{};
    uint64_t ack{};
    uint32_t peerId{};
    Delivery delivery{Delivery::Unreliable};
    uint16_t size{};
};

struct TransportStats {
    uint64_t sent{};
    uint64_t received{};
    uint64_t dropped{};
    uint64_t duplicated{};
    uint64_t reordered{};
    uint64_t rejected{};
};

class PacketWindow {
public:
    bool accept(uint64_t sequence) {
        if (!sequence) return false;
        if (!initialized_) { initialized_=true; newest_=sequence; bits_=1; return true; }
        if (sequence > newest_) {
            const auto delta=sequence-newest_;
            bits_=delta>=64 ? 1ULL : ((bits_<<delta)|1ULL);
            newest_=sequence;
            return true;
        }
        const auto age=newest_-sequence;
        if (age>=64) return false;
        const auto bit=1ULL<<age;
        if (bits_&bit) return false;
        bits_|=bit;
        return true;
    }
private:
    bool initialized_{};
    uint64_t newest_{};
    uint64_t bits_{};
};

class TransportQueue {
public:
    static constexpr uint16_t Capacity=512;

    bool enqueue(Packet packet) {
        if (!packet.peerId || !packet.sequence || packet.size==0 || count_>=Capacity) {
            ++stats_.rejected; return false;
        }
        queue_[(head_+count_)%Capacity]=packet;
        ++count_; ++stats_.sent;
        return true;
    }

    bool dequeue(Packet& packet) {
        if (!count_) return false;
        packet=queue_[head_];
        head_=(head_+1)%Capacity;
        --count_; ++stats_.received;
        return true;
    }

    bool acknowledge(uint64_t sequence) {
        if (!sequence || sequence>lastSent_) return false;
        lastAck_=sequence;
        return true;
    }

    void recordSentSequence(uint64_t sequence) { if (sequence>lastSent_) lastSent_=sequence; }
    void recordDrop() { ++stats_.dropped; }
    void recordDuplicate() { ++stats_.duplicated; }
    void recordReorder() { ++stats_.reordered; }
    [[nodiscard]] uint16_t size() const { return count_; }
    [[nodiscard]] uint64_t lastAck() const { return lastAck_; }
    [[nodiscard]] const TransportStats& stats() const { return stats_; }

private:
    std::array<Packet,Capacity> queue_{};
    uint16_t head_{};
    uint16_t count_{};
    uint64_t lastSent_{};
    uint64_t lastAck_{};
    TransportStats stats_{};
};

} // namespace NeoEngine::Networking
