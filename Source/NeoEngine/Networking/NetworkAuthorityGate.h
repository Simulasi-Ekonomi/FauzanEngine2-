#pragma once
#include <cstdint>

namespace NeoEngine::Networking {

struct AuthorityCommand {
    uint64_t peerId{};
    uint64_t actorId{};
    uint64_t sequence{};
    uint32_t commandType{};
    uint32_t payloadBytes{};
};

enum class AuthorityReject : uint8_t {
    None,
    InvalidPeer,
    InvalidActor,
    NotOwner,
    InvalidSequence,
    PayloadTooLarge,
    RateLimited
};

struct AuthorityResult {
    bool accepted{false};
    AuthorityReject reason{AuthorityReject::None};
};

class AuthorityGate {
public:
    static constexpr uint32_t MaxPayloadBytes = 64 * 1024;
    static constexpr uint32_t MaxCommandsPerWindow = 120;

    void reset(uint64_t peerId, uint64_t actorId) {
        peerId_ = peerId;
        actorId_ = actorId;
        lastSequence_ = 0;
        windowCount_ = 0;
    }

    AuthorityResult validate(const AuthorityCommand& command) {
        if (!peerId_ || command.peerId != peerId_) return {false, AuthorityReject::InvalidPeer};
        if (!actorId_ || command.actorId != actorId_) return {false, AuthorityReject::NotOwner};
        if (!command.sequence || command.sequence <= lastSequence_) return {false, AuthorityReject::InvalidSequence};
        if (command.payloadBytes > MaxPayloadBytes) return {false, AuthorityReject::PayloadTooLarge};
        if (windowCount_ >= MaxCommandsPerWindow) return {false, AuthorityReject::RateLimited};
        lastSequence_ = command.sequence;
        ++windowCount_;
        return {true, AuthorityReject::None};
    }

    void beginRateWindow() { windowCount_ = 0; }
    uint64_t lastSequence() const { return lastSequence_; }
    uint32_t windowCount() const { return windowCount_; }

private:
    uint64_t peerId_{};
    uint64_t actorId_{};
    uint64_t lastSequence_{};
    uint32_t windowCount_{};
};

} // namespace NeoEngine::Networking
