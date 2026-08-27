#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace NeoEngine::Networking {

struct SessionResumeToken {
    uint64_t sessionId{0};
    uint64_t peerId{0};
    uint64_t lastAcknowledgedSequence{0};

    bool valid() const { return sessionId != 0 && peerId != 0; }

    bool operator==(const SessionResumeToken& other) const {
        return sessionId == other.sessionId && peerId == other.peerId &&
               lastAcknowledgedSequence == other.lastAcknowledgedSequence;
    }
};

class SessionResumeState {
public:
    bool establish(uint64_t sessionId, uint64_t peerId) {
        if (sessionId == 0 || peerId == 0) return false;
        token_ = {sessionId, peerId, 0};
        return true;
    }

    bool acknowledge(uint64_t sequence) {
        if (!token_.valid() || sequence < token_.lastAcknowledgedSequence) return false;
        token_.lastAcknowledgedSequence = sequence;
        return true;
    }

    bool canResume(const SessionResumeToken& candidate) const {
        return token_.valid() && candidate.valid() &&
               candidate.sessionId == token_.sessionId &&
               candidate.peerId == token_.peerId &&
               candidate.lastAcknowledgedSequence <= token_.lastAcknowledgedSequence;
    }

    void clear() { token_ = {}; }
    const SessionResumeToken& token() const { return token_; }

private:
    SessionResumeToken token_{};
};

} // namespace NeoEngine::Networking
