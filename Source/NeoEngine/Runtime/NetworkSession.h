#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace NeoEngine {

enum class NetworkRole : uint8_t { Server, Client };
enum class NetworkError : uint8_t { None, InvalidPeer, InvalidInput, Stale, Duplicate, Ownership, Capacity, Checksum };

struct NetworkInputCommand { uint32_t clientId{}; uint32_t networkId{}; uint64_t sequence{}; uint64_t clientTick{}; float moveX{}; float moveZ{}; };
struct NetworkTransformState { uint32_t networkId{}; uint32_t ownerId{}; uint64_t revision{}; float x{}; float y{}; float z{}; };
struct NetworkSnapshot { uint64_t sequence{}; uint64_t serverTick{}; std::vector<NetworkTransformState> states; uint64_t checksum{}; };
struct NetworkReconciliationReceipt { bool reconciled{}; uint32_t replayedInputs{}; uint64_t acknowledgedInput{}; uint64_t authoritativeRevision{}; float correctionDistance{}; };

class NetworkSequenceWindow {
public:
    bool accept(uint64_t sequence) {
        if (!sequence) return false;
        if (!initialized_) { initialized_=true; newest_=sequence; bits_=1; return true; }
        if (sequence>newest_) { auto d=sequence-newest_; bits_=d>=64?1ULL:(bits_<<d)|1ULL; newest_=sequence; return true; }
        auto age=newest_-sequence; if (age>=64) return false; auto bit=1ULL<<age; if(bits_&bit) return false; bits_|=bit; return true;
    }
private: bool initialized_{}; uint64_t newest_{}; uint64_t bits_{};
};

class NetworkPredictionBuffer {
public:
    static constexpr uint16_t MaxFrames=256;
    bool record(const NetworkInputCommand& in,float x,float y,float z){ if(!in.sequence||count_>=MaxFrames)return false; frames_[(head_+count_)%MaxFrames]={in,x,y,z};++count_;return true; }
    bool full() const { return count_ >= MaxFrames; }
    template<class Sim> uint32_t replay(uint64_t acknowledged,float& x,float& y,float& z,Sim&& simulate){ uint32_t n=0; while(count_&&frames_[head_].input.sequence<=acknowledged){head_=(head_+1)%MaxFrames;--count_;} for(uint16_t i=0;i<count_;++i){auto const& f=frames_[(head_+i)%MaxFrames];simulate(f.input,x,y,z);++n;} return n; }
private:
    struct Frame{NetworkInputCommand input{};float x{},y{},z{};}; std::array<Frame,MaxFrames> frames_{}; uint16_t head_{}; uint16_t count_{};
};

class NetworkSession {
public:
    explicit NetworkSession(NetworkRole role,uint32_t peer):role_(role),peerId_(peer){}
    bool initialize(){if(peerId_!=0){error_=NetworkError::None;return true;}return Fail(NetworkError::InvalidPeer);}
    bool registerPeer(uint32_t peer){if(!peer||peer==peerId_)return Fail(NetworkError::InvalidPeer);for(auto p:peers_)if(p==peer){error_=NetworkError::None;return true;}for(auto& p:peers_)if(!p){p=peer;error_=NetworkError::None;return true;}return Fail(NetworkError::Capacity);}
    bool assignOwnership(uint32_t id,uint32_t owner){if(role_!=NetworkRole::Server||!id||!owner)return Fail(NetworkError::InvalidInput);if(!isRegisteredPeer(owner))return Fail(NetworkError::InvalidPeer);for(auto&e:entities_)if(e.networkId==id||!e.networkId){if(!e.networkId)e.networkId=id;e.ownerId=owner;error_=NetworkError::None;return true;}return Fail(NetworkError::Capacity);}
    bool serverConsume(const NetworkInputCommand& in,NetworkTransformState& out){if(role_!=NetworkRole::Server)return Fail(NetworkError::InvalidInput);if(!validateInput(in))return false;for(auto&e:entities_)if(e.networkId==in.networkId){if(e.ownerId!=in.clientId)return Fail(NetworkError::Ownership);if(!acceptSequence(in.clientId,in.sequence))return false;e.x+=in.moveX;e.z+=in.moveZ;++e.revision;out=e;error_=NetworkError::None;return true;}return Fail(NetworkError::InvalidInput);}
    bool predict(const NetworkInputCommand& in){if(role_!=NetworkRole::Client||in.clientId!=peerId_)return Fail(NetworkError::InvalidPeer);if(prediction_.full())return Fail(NetworkError::Capacity);if(!accept(in))return false;const float nextX=predictedX_+in.moveX,nextZ=predictedZ_+in.moveZ;if(!prediction_.record(in,nextX,predictedY_,nextZ))return Fail(NetworkError::Capacity);predictedX_=nextX;predictedZ_=nextZ;error_=NetworkError::None;return true;}
    bool reconcile(const NetworkTransformState& authoritative,uint64_t acknowledged,NetworkReconciliationReceipt& r){if(role_!=NetworkRole::Client||authoritative.ownerId!=peerId_)return Fail(NetworkError::InvalidPeer);if(authoritative.revision<revision_)return Fail(NetworkError::Stale);revision_=authoritative.revision;float dx=predictedX_-authoritative.x,dy=predictedY_-authoritative.y,dz=predictedZ_-authoritative.z;r={true,0,acknowledged,revision_,dx*dx+dy*dy+dz*dz};predictedX_=authoritative.x;predictedY_=authoritative.y;predictedZ_=authoritative.z;r.replayedInputs=prediction_.replay(acknowledged,predictedX_,predictedY_,predictedZ_,[](auto const&i,float&x,float&,float&z){x+=i.moveX;z+=i.moveZ;});error_=NetworkError::None;return true;}
    NetworkSnapshot snapshot(uint64_t tick)const{NetworkSnapshot s; s.sequence=++snapshotSequence_;s.serverTick=tick;for(auto const&e:entities_)if(e.networkId)s.states.push_back(e);s.checksum=checksum(s);return s;}
    bool acceptSnapshot(NetworkSnapshot const&s){if(role_!=NetworkRole::Client)return Fail(NetworkError::InvalidInput);if(s.sequence<=lastSnapshot_)return Fail(NetworkError::Duplicate);if(checksum(s)!=s.checksum)return Fail(NetworkError::Checksum);lastSnapshot_=s.sequence;error_=NetworkError::None;return true;}
    [[nodiscard]] NetworkError lastError()const{return error_;}
private:
    struct PeerWindow { uint32_t peerId{}; NetworkSequenceWindow sequence{}; };
    bool Fail(NetworkError error){error_=error;return false;}
    bool isRegisteredPeer(uint32_t peer) const { for(auto p:peers_) if(p==peer) return true; return false; }
    bool validateInput(NetworkInputCommand const&i){if(!i.clientId||!i.networkId||!i.sequence||i.moveX!=i.moveX||i.moveZ!=i.moveZ)return Fail(NetworkError::InvalidInput);float m=i.moveX*i.moveX+i.moveZ*i.moveZ;if(m>1.001f)return Fail(NetworkError::InvalidInput);return true;}
    bool acceptSequence(uint32_t peerId,uint64_t sequence){for(auto& p:windows_)if(p.peerId==peerId){if(!p.sequence.accept(sequence))return Fail(NetworkError::Duplicate);error_=NetworkError::None;return true;}for(auto& p:windows_)if(!p.peerId){NetworkSequenceWindow candidate;if(!candidate.accept(sequence))return Fail(NetworkError::Duplicate);p.peerId=peerId;p.sequence=candidate;error_=NetworkError::None;return true;}return Fail(NetworkError::Capacity);}
    bool accept(NetworkInputCommand const&i){if(!validateInput(i))return false;return acceptSequence(i.clientId,i.sequence);}
    static uint64_t checksum(NetworkSnapshot const&s){uint64_t h=1469598103934665603ULL;auto mix=[&](uint64_t v){for(int i=0;i<8;++i){h^=(v>>(i*8))&255ULL;h*=1099511628211ULL;}};mix(s.sequence);mix(s.serverTick);for(auto const&e:s.states){mix(e.networkId);mix(e.ownerId);mix(e.revision);uint32_t a,b,c;__builtin_memcpy(&a,&e.x,4);__builtin_memcpy(&b,&e.y,4);__builtin_memcpy(&c,&e.z,4);mix(a);mix(b);mix(c);}return h;}
    NetworkRole role_;uint32_t peerId_{};NetworkError error_{};std::array<uint32_t,128> peers_{};std::array<PeerWindow,128> windows_{};std::array<NetworkTransformState,256> entities_{};mutable uint64_t snapshotSequence_{};uint64_t lastSnapshot_{};uint64_t revision_{};NetworkPredictionBuffer prediction_{};float predictedX_{},predictedY_{},predictedZ_{};
};
}
