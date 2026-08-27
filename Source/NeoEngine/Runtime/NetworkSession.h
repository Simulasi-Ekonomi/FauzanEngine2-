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
    bool initialize(){return peerId_!=0;}
    bool registerPeer(uint32_t peer){ if(!peer||peer==peerId_)return false; for(auto p:peers_)if(p==peer)return true;for(auto& p:peers_)if(!p){p=peer;return true;}return false; }
    bool assignOwnership(uint32_t id,uint32_t owner){if(role_!=NetworkRole::Server||!id||!owner)return false;for(auto&e:entities_)if(e.networkId==id||!e.networkId){if(!e.networkId)e.networkId=id;e.ownerId=owner;return true;}return false;}
    bool serverConsume(const NetworkInputCommand& in,NetworkTransformState& out){if(role_!=NetworkRole::Server||!accept(in))return false;for(auto&e:entities_)if(e.networkId==in.networkId){if(e.ownerId!=in.clientId)return false;e.x+=in.moveX;e.z+=in.moveZ;++e.revision;out=e;return true;}return false;}
    bool predict(const NetworkInputCommand& in){if(role_!=NetworkRole::Client||in.clientId!=peerId_||prediction_.full()||!accept(in))return false;const float nextX=predictedX_+in.moveX,nextZ=predictedZ_+in.moveZ;if(!prediction_.record(in,nextX,predictedY_,nextZ))return false;predictedX_=nextX;predictedZ_=nextZ;return true;}
    bool reconcile(const NetworkTransformState& authoritative,uint64_t acknowledged,NetworkReconciliationReceipt& r){if(role_!=NetworkRole::Client||authoritative.ownerId!=peerId_||authoritative.revision<revision_)return false;revision_=authoritative.revision;float dx=predictedX_-authoritative.x,dy=predictedY_-authoritative.y,dz=predictedZ_-authoritative.z;r={true,0,acknowledged,revision_,dx*dx+dy*dy+dz*dz};predictedX_=authoritative.x;predictedY_=authoritative.y;predictedZ_=authoritative.z;r.replayedInputs=prediction_.replay(acknowledged,predictedX_,predictedY_,predictedZ_,[](auto const&i,float&x,float&,float&z){x+=i.moveX;z+=i.moveZ;});return true;}
    NetworkSnapshot snapshot(uint64_t tick)const{NetworkSnapshot s; s.sequence=++snapshotSequence_;s.serverTick=tick;for(auto const&e:entities_)if(e.networkId)s.states.push_back(e);s.checksum=checksum(s);return s;}
    bool acceptSnapshot(NetworkSnapshot const&s){if(role_!=NetworkRole::Client||s.sequence<=lastSnapshot_)return false;if(checksum(s)!=s.checksum)return false;lastSnapshot_=s.sequence;return true;}
    [[nodiscard]] NetworkError lastError()const{return error_;}
private:
    bool accept(NetworkInputCommand const&i){if(!i.clientId||!i.networkId||!i.sequence||i.moveX!=i.moveX||i.moveZ!=i.moveZ)return false;float m=i.moveX*i.moveX+i.moveZ*i.moveZ;if(m>1.001f)return false;return windows_[i.clientId%128].accept(i.sequence);}
    static uint64_t checksum(NetworkSnapshot const&s){uint64_t h=1469598103934665603ULL;auto mix=[&](uint64_t v){for(int i=0;i<8;++i){h^=(v>>(i*8))&255ULL;h*=1099511628211ULL;}};mix(s.sequence);mix(s.serverTick);for(auto const&e:s.states){mix(e.networkId);mix(e.ownerId);mix(e.revision);uint32_t a,b,c;__builtin_memcpy(&a,&e.x,4);__builtin_memcpy(&b,&e.y,4);__builtin_memcpy(&c,&e.z,4);mix(a);mix(b);mix(c);}return h;}
    NetworkRole role_;uint32_t peerId_{};NetworkError error_{};std::array<uint32_t,128> peers_{};std::array<NetworkSequenceWindow,128> windows_{};std::array<NetworkTransformState,256> entities_{};mutable uint64_t snapshotSequence_{};uint64_t lastSnapshot_{};uint64_t revision_{};NetworkPredictionBuffer prediction_{};float predictedX_{},predictedY_{},predictedZ_{};
};
}
