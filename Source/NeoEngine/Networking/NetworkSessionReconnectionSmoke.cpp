#include <cassert>
#include <cstdint>
#include "NetworkReconnectPolicy.h"
#include "NetworkSessionReconnection.h"

using namespace NeoEngine::Networking;

int main() {
    ReconnectPolicy policy;
    assert(policy.next(1).allowed && policy.next(1).delayMs == 250U);
    assert(policy.next(2).allowed && policy.next(2).delayMs == 500U);
    assert(policy.next(6).allowed && policy.next(6).delayMs == 8000U);
    assert(!policy.next(0).allowed);
    assert(!policy.next(9).allowed);

    SessionReconnection session;
    assert(session.state() == ReconnectionState::Disconnected);
    assert(session.transition(ReconnectionEvent::Begin));
    assert(session.state() == ReconnectionState::Connecting);
    assert(!session.transition(ReconnectionEvent::Begin));
    assert(session.transition(ReconnectionEvent::Connected));
    assert(session.state() == ReconnectionState::Connected);
    assert(session.transition(ReconnectionEvent::ConnectionLost));
    assert(session.state() == ReconnectionState::Backoff);
    assert(session.transition(ReconnectionEvent::RetryTimer));
    assert(session.state() == ReconnectionState::Connecting);
    assert(session.attempts() == 1U);
    assert(session.delayMs() == 250U);
    assert(session.transition(ReconnectionEvent::RetryFailed));
    assert(session.state() == ReconnectionState::Backoff);
    assert(session.transition(ReconnectionEvent::RetryTimer));
    assert(session.attempts() == 2U);
    assert(session.delayMs() == 500U);
    assert(session.transition(ReconnectionEvent::Connected));
    assert(session.state() == ReconnectionState::Connected);
    assert(session.attempts() == 0U);
    assert(session.delayMs() == 0U);
    assert(session.transition(ReconnectionEvent::Reset));
    assert(session.state() == ReconnectionState::Disconnected);

    std::uint8_t limitedMax = 2;
    SessionReconnection limited(limitedMax);
    assert(limited.transition(ReconnectionEvent::Begin));
    assert(limited.transition(ReconnectionEvent::Connected));
    assert(limited.transition(ReconnectionEvent::ConnectionLost));
    assert(limited.transition(ReconnectionEvent::RetryTimer));
    assert(limited.transition(ReconnectionEvent::RetryFailed));
    assert(limited.transition(ReconnectionEvent::RetryTimer));
    assert(limited.transition(ReconnectionEvent::RetryFailed));
    assert(limited.state() == ReconnectionState::Exhausted);
    assert(!limited.transition(ReconnectionEvent::RetryTimer));

    return 0;
}
