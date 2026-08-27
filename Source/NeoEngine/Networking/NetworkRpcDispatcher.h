#pragma once
#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace NeoEngine::Networking {

enum class RpcDirection : uint8_t { ServerToClient, ClientToServer };

struct RpcEnvelope {
    uint64_t peerId{};
    uint64_t actorId{};
    uint64_t sequence{};
    uint32_t rpcId{};
    RpcDirection direction{RpcDirection::ClientToServer};
    uint32_t payloadBytes{};
};

struct RpcDispatchResult {
    bool accepted{false};
    enum class Reason : uint8_t { None, UnknownRpc, DirectionDenied, DuplicateSequence, PayloadTooLarge, InvalidPeer, InvalidSequence, HandlerRejected } reason{Reason::None};
};

class RpcDispatcher {
public:
    using Handler = bool(*)(const RpcEnvelope&);
    static constexpr uint32_t MaxPayloadBytes = 64 * 1024;

    bool registerRpc(uint32_t rpcId, RpcDirection direction, Handler handler) {
        if (!rpcId || !handler || handlers_.count(rpcId)) return false;
        try { handlers_.emplace(rpcId, Entry{direction, handler}); return true; } catch (...) { return false; }
    }

    RpcDispatchResult dispatch(const RpcEnvelope& rpc) {
        if (!rpc.peerId) return {false, RpcDispatchResult::Reason::InvalidPeer};
        if (!rpc.sequence) return {false, RpcDispatchResult::Reason::InvalidSequence};
        const auto it = handlers_.find(rpc.rpcId);
        if (it == handlers_.end()) return {false, RpcDispatchResult::Reason::UnknownRpc};
        if (it->second.direction != rpc.direction) return {false, RpcDispatchResult::Reason::DirectionDenied};
        if (rpc.payloadBytes > MaxPayloadBytes) return {false, RpcDispatchResult::Reason::PayloadTooLarge};
        const SequenceKey key{rpc.peerId, rpc.rpcId};
        const auto last = lastSequences_.find(key);
        if (last != lastSequences_.end() && rpc.sequence <= last->second)
            return {false, RpcDispatchResult::Reason::DuplicateSequence};
        bool handled = false;
        try { handled = it->second.handler(rpc); } catch (...) { return {false, RpcDispatchResult::Reason::HandlerRejected}; }
        if (!handled) return {false, RpcDispatchResult::Reason::HandlerRejected};
        lastSequences_[key] = rpc.sequence;
        return {true, RpcDispatchResult::Reason::None};
    }

    void clearPeer(uint64_t peerId) {
        for (auto it = lastSequences_.begin(); it != lastSequences_.end();) {
            if (it->first.peerId == peerId) it = lastSequences_.erase(it); else ++it;
        }
    }

private:
    struct Entry { RpcDirection direction; Handler handler; };
    struct SequenceKey { uint64_t peerId; uint32_t rpcId; bool operator==(const SequenceKey& other) const { return peerId == other.peerId && rpcId == other.rpcId; } };
    struct SequenceKeyHash { std::size_t operator()(const SequenceKey& key) const { return static_cast<std::size_t>((key.peerId * 0x9E3779B97F4A7C15ULL) ^ key.rpcId); } };
    std::unordered_map<uint32_t, Entry> handlers_;
    std::unordered_map<SequenceKey, uint64_t, SequenceKeyHash> lastSequences_;
};

} // namespace NeoEngine::Networking
