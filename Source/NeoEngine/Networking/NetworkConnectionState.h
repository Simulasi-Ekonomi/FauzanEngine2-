#pragma once
#include <cstdint>

namespace NeoEngine::Networking {

enum class ConnectionState : uint8_t { Disconnected, Connecting, Authenticating, Connected, Closing, Closed, Failed };
enum class ConnectionEvent : uint8_t { Begin, ChallengeAccepted, HandshakeFailed, DisconnectRequested, PeerClosed, Timeout };

class ConnectionStateMachine {
public:
    bool transition(ConnectionEvent event){
        switch(event){
            case ConnectionEvent::Begin: if(state_!=ConnectionState::Disconnected&&state_!=ConnectionState::Closed)return false;state_=ConnectionState::Connecting;return true;
            case ConnectionEvent::ChallengeAccepted: if(state_!=ConnectionState::Connecting)return false;state_=ConnectionState::Connected;return true;
            case ConnectionEvent::HandshakeFailed: if(state_!=ConnectionState::Connecting&&state_!=ConnectionState::Authenticating)return false;state_=ConnectionState::Failed;return true;
            case ConnectionEvent::DisconnectRequested: if(state_!=ConnectionState::Connected)return false;state_=ConnectionState::Closing;return true;
            case ConnectionEvent::PeerClosed: if(state_!=ConnectionState::Closing&&state_!=ConnectionState::Connected)return false;state_=ConnectionState::Closed;return true;
            case ConnectionEvent::Timeout: if(state_==ConnectionState::Disconnected||state_==ConnectionState::Closed)return false;state_=ConnectionState::Failed;return true;
        }
        return false;
    }
    [[nodiscard]] ConnectionState state()const{return state_;}
private: ConnectionState state_{ConnectionState::Disconnected};
};
}
