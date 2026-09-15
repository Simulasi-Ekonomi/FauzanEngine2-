#pragma once

#include <cstdint>

namespace NeoEngine {

enum class AndroidLifecycleState : uint8_t { Created, Resumed, Paused, Stopped, Destroyed };

class AndroidLifecycleGate final {
public:
    AndroidLifecycleState State() const { return m_State; }
    bool OnResume() { return Transition(AndroidLifecycleState::Resumed); }
    bool OnPause() { return Transition(AndroidLifecycleState::Paused); }
    bool OnStop() { return Transition(AndroidLifecycleState::Stopped); }
    bool OnDestroy() { return Transition(AndroidLifecycleState::Destroyed); }
    bool CanRender() const { return m_State == AndroidLifecycleState::Resumed; }
    bool CanSubmitAudio() const { return m_State == AndroidLifecycleState::Resumed || m_State == AndroidLifecycleState::Paused; }

private:
    bool Transition(AndroidLifecycleState next) {
        if (m_State == AndroidLifecycleState::Destroyed) return false;
        if (m_State == AndroidLifecycleState::Created && next != AndroidLifecycleState::Resumed && next != AndroidLifecycleState::Destroyed) return false;
        if (m_State == AndroidLifecycleState::Resumed && next != AndroidLifecycleState::Paused && next != AndroidLifecycleState::Stopped && next != AndroidLifecycleState::Destroyed) return false;
        if (m_State == AndroidLifecycleState::Paused && next != AndroidLifecycleState::Resumed && next != AndroidLifecycleState::Stopped && next != AndroidLifecycleState::Destroyed) return false;
        if (m_State == AndroidLifecycleState::Stopped && next != AndroidLifecycleState::Resumed && next != AndroidLifecycleState::Destroyed) return false;
        m_State = next;
        return true;
    }

    AndroidLifecycleState m_State = AndroidLifecycleState::Created;
};

} // namespace NeoEngine
