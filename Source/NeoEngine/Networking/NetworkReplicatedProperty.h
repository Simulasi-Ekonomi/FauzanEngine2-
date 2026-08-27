#pragma once
#include <array>
#include <cstdint>

namespace NeoEngine::Networking {

enum class ReplicatedType : uint8_t { UInt32, Float, Bool };
struct ReplicatedProperty { uint16_t id{}; ReplicatedType type{}; uint32_t ownerPeer{}; uint64_t revision{}; bool dirty{}; uint32_t u32{}; float f32{}; bool b{}; };

class ReplicatedPropertySet {
public:
    static constexpr uint16_t Capacity=256;
    bool define(uint16_t id,ReplicatedType type,uint32_t ownerPeer=0){if(!id||find(id))return false;for(auto&p:properties_)if(!p.id){p={id,type,ownerPeer,0,false,0,0.0F,false};return true;}return false;}
    bool setUInt32(uint16_t id,uint32_t value,uint32_t writer){auto*p=findWritable(id,writer,ReplicatedType::UInt32);if(!p)return false;if(p->u32==value)return true;p->u32=value;return markDirty(*p);}
    bool setFloat(uint16_t id,float value,uint32_t writer){auto*p=findWritable(id,writer,ReplicatedType::Float);if(!p)return false;if(p->f32==value)return true;p->f32=value;return markDirty(*p);}
    bool setBool(uint16_t id,bool value,uint32_t writer){auto*p=findWritable(id,writer,ReplicatedType::Bool);if(!p)return false;if(p->b==value)return true;p->b=value;return markDirty(*p);}
    uint16_t collectDirty(uint16_t*ids,uint16_t capacity){if(!ids)return 0;uint16_t n=0;for(auto&p:properties_)if(p.id&&p.dirty&&n<capacity){ids[n++]=p.id;p.dirty=false;}return n;}
private:
    static bool markDirty(ReplicatedProperty&p){++p.revision;p.dirty=true;return true;}
    ReplicatedProperty* findWritable(uint16_t id,uint32_t writer,ReplicatedType type){auto*p=find(id);if(!p||p->type!=type||(!writer||p->ownerPeer!=0&&p->ownerPeer!=writer))return nullptr;return p;}
    ReplicatedProperty* find(uint16_t id){for(auto&p:properties_)if(p.id==id)return &p;return nullptr;}
    std::array<ReplicatedProperty,Capacity> properties_{};
};
}
