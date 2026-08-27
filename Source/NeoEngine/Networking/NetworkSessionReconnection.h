#pragma once
#include <cstdint>
#include "NetworkReconnectPolicy.h"

namespace NeoEngine::Networking {

enum class ReconnectionState : uint8_t { Disconnected, Connecting, Connected, Backoff, Exhausted };
enum class ReconnectionEvent : uint8_t { Begin, Connected, ConnectionLost, RetryTimer, RetryFailed, Reset };

class SessionReconnection {
public:
    explicit SessionReconnection(uint8_t maxAttempts = 8)
        : policy_(250, 8000, maxAttempts), maxAttempts_(maxAttempts) {}

    bool transition(ReconnectionEvent event) {
        switch (event) {
            case ReconnectionEvent::Begin:
                if (state_ != ReconnectionState::Disconnected && state_ != ReconnectionState::Exhausted) return false;
                attempts_ = 0;
                state_ = ReconnectionState::Connecting;
                return true;
            case ReconnectionEvent::Connected:
                if (state_ != ReconnectionState::Connecting && state_ != ReconnectionState::Backoff) return false;
                attempts_ = 0;
                state_ = ReconnectionState::Connected;
                return true;
            case ReconnectionEvent::ConnectionLost:
                if (state_ != ReconnectionState::Connected) return false;
                state_ = ReconnectionState::Backoff;
                return true;
            case ReconnectionEvent::RetryTimer:
                if (state_ != ReconnectionState::Backoff || attempts_ >= maxAttempts_) {
                    if (attempts_ >= maxAttempts_) state_ = ReconnectionState::Exhausted;
                    return false;
                }
                ++attempts_;
                state_ = ReconnectionState::Connecting;
                return true;
            case ReconnectionEvent::RetryFailed:
                if (state_ != ReconnectionState::Connecting) return false;
                state_ = attempts_ >= maxAttempts_ ? ReconnectionState::Exhausted : ReconnectionState::Backoff;
                return true;
            case ReconnectionEvent::Reset:
                attempts_ = 0;
                state_ = ReconnectionState::Disconnected;
                return true;
        }
        return false;
    }

    ReconnectionState state() const { return state_; }
    uint8_t attempts() const { return attempts_; }
    uint32_t delayMs() const {
        return attempts_ == 0 ? 0U : policy_.next(attempts_).delayMs;
    }

private:
    ReconnectPolicy policy_;
    ReconnectionState state_{ReconnectionState::Disconnected};
    uint8_t attempts_{};
    uint8_t maxAttempts_{};
};
}
