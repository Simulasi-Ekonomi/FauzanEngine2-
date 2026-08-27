#pragma once
#include <cstdint>

namespace NeoEngine::Networking {

enum class HandshakeState : uint8_t { Disconnected, HelloSent, ChallengeSent, Established, Rejected };
enum class HandshakeResult : uint8_t { None, Accepted, InvalidVersion, InvalidPeer, Timeout, Capacity };

struct HandshakeHello { uint16_t protocolVersion{1}; uint32_t peerId{}; uint64_t nonce{}; };
struct HandshakeChallenge { uint16_t protocolVersion{1}; uint64_t nonce{}; uint64_t challenge{}; };

class HandshakeMachine {
public:
    static constexpr uint16_t ProtocolVersion=1;
    bool begin(uint32_t peerId,uint64_t nonce){if(!peerId||!nonce)return false;peerId_=peerId;nonce_=nonce;state_=HandshakeState::HelloSent;result_=HandshakeResult::None;return true;}
    bool receiveHello(const HandshakeHello& hello,uint64_t challenge){if(!hello.peerId||hello.protocolVersion!=ProtocolVersion||!challenge){state_=HandshakeState::Rejected;result_=hello.protocolVersion==ProtocolVersion?HandshakeResult::InvalidPeer:HandshakeResult::InvalidVersion;return false;}peerId_=hello.peerId;nonce_=hello.nonce;challenge_=challenge;state_=HandshakeState::ChallengeSent;return true;}
    bool acceptChallenge(const HandshakeChallenge& challenge){if(state_!=HandshakeState::HelloSent||challenge.protocolVersion!=ProtocolVersion||challenge.nonce!=nonce_||!challenge.challenge){state_=HandshakeState::Rejected;result_=HandshakeResult::InvalidVersion;return false;}challenge_=challenge.challenge;state_=HandshakeState::Established;result_=HandshakeResult::Accepted;return true;}
    bool establish(uint64_t response){if(state_!=HandshakeState::ChallengeSent||response!=challenge_){state_=HandshakeState::Rejected;result_=HandshakeResult::InvalidPeer;return false;}state_=HandshakeState::Established;result_=HandshakeResult::Accepted;return true;}
    [[nodiscard]] HandshakeState state()const{return state_;}
    [[nodiscard]] HandshakeResult result()const{return result_;}
    [[nodiscard]] uint32_t peerId()const{return peerId_;}
private:
    HandshakeState state_{HandshakeState::Disconnected};HandshakeResult result_{};uint32_t peerId_{};uint64_t nonce_{},challenge_{};
};
}
