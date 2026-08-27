#pragma once
#include <array>
#include <cstdint>

namespace NeoEngine::Networking {

enum class RpcDirection : uint8_t { ServerOnly, ClientToServer, ServerToClient, Bidirectional };
struct RpcRequest { uint32_t peerId{}; uint16_t rpcId{}; uint32_t payloadBytes{}; uint64_t sequence{}; };
using RpcHandler = bool(*)(const RpcRequest&);

class RpcRegistry {
public:
    static constexpr uint16_t Capacity=256; static constexpr uint32_t MaxPayload=1200;
    struct Entry { uint16_t id{}; RpcDirection direction{}; RpcHandler handler{}; };
    bool registerRpc(uint16_t id,RpcDirection direction,RpcHandler handler){if(!id||!handler||find(id))return false;for(auto&e:entries_)if(!e.id){e={id,direction,handler};++count_;return true;}return false;}
    bool dispatch(const RpcRequest& request,RpcDirection incoming)const{if(!request.peerId||!request.rpcId||request.payloadBytes>MaxPayload)return false;auto*e=find(request.rpcId);if(!e||!allowed(e->direction,incoming))return false;return e->handler(request);}
    uint16_t count()const{return count_;}
private:
    static bool allowed(RpcDirection configured,RpcDirection incoming){return configured==RpcDirection::Bidirectional||configured==incoming||incoming==RpcDirection::Bidirectional;}
    const Entry* find(uint16_t id)const{for(auto const&e:entries_)if(e.id==id)return &e;return nullptr;}
    std::array<Entry,Capacity> entries_{};uint16_t count_{};
};
}
