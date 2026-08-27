#pragma once
#include <cstdint>
#include <vector>

namespace NeoEngine::Networking {

struct SnapshotState { uint32_t networkId{}; uint32_t ownerId{}; uint64_t revision{}; float x{},y{},z{}; };
struct SnapshotDelta { uint32_t networkId{}; uint8_t mask{}; SnapshotState state{}; };

enum FieldMask : uint8_t { Position=1, Owner=2, Revision=4 };

class SnapshotDeltaCodec {
public:
    static SnapshotDelta makeDelta(const SnapshotState& before,const SnapshotState& after){
        SnapshotDelta d{}; d.networkId=after.networkId; d.state=after;
        if(before.x!=after.x||before.y!=after.y||before.z!=after.z)d.mask|=Position;
        if(before.ownerId!=after.ownerId)d.mask|=Owner;
        if(before.revision!=after.revision)d.mask|=Revision;
        return d;
    }
    static bool apply(const SnapshotDelta& d,SnapshotState& target){
        if(!d.networkId||d.state.networkId!=d.networkId||!d.mask)return false;
        if(d.mask&Position){target.x=d.state.x;target.y=d.state.y;target.z=d.state.z;}
        if(d.mask&Owner)target.ownerId=d.state.ownerId;
        if(d.mask&Revision)target.revision=d.state.revision;
        target.networkId=d.networkId; return true;
    }
    static std::vector<SnapshotDelta> build(const SnapshotState* previous,const SnapshotState* current,uint16_t count){
        std::vector<SnapshotDelta> out; if(!current)return out;
        for(uint16_t i=0;i<count;++i){auto d=makeDelta(previous?previous[i]:SnapshotState{},current[i]);if(d.mask)out.push_back(d);} return out;
    }
};
}
