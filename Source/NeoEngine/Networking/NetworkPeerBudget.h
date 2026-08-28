#pragma once
#include <cstddef>
#include <cstdint>

namespace NeoEngine::Networking {

struct NetworkBudgetLimits {
    uint32_t packetsPerWindow{120};
    uint32_t bytesPerWindow{262144};
    uint32_t commandsPerWindow{60};
};

struct NetworkBudgetUsage {
    uint32_t packets{};
    uint32_t bytes{};
    uint32_t commands{};
};

class NetworkPeerBudget {
public:
    explicit NetworkPeerBudget(NetworkBudgetLimits limits = {}) : limits_(limits) {}

    void resetWindow() { usage_ = {}; violations_ = 0; }

    bool consumePacket(std::size_t bytes) {
        if (bytes > UINT32_MAX) return false;
        if (usage_.packets >= limits_.packetsPerWindow) return reject();
        if (usage_.bytes > limits_.bytesPerWindow || bytes > limits_.bytesPerWindow - usage_.bytes) return reject();
        ++usage_.packets;
        usage_.bytes += static_cast<uint32_t>(bytes);
        return true;
    }

    bool consumeCommand() {
        if (usage_.commands >= limits_.commandsPerWindow) return reject();
        ++usage_.commands;
        return true;
    }

    const NetworkBudgetUsage& usage() const { return usage_; }
    uint32_t violations() const { return violations_; }
    const NetworkBudgetLimits& limits() const { return limits_; }

private:
    bool reject() { ++violations_; return false; }
    NetworkBudgetLimits limits_;
    NetworkBudgetUsage usage_;
    uint32_t violations_{};
};

} // namespace NeoEngine::Networking
