#pragma once
#include <array>
#include <cstdint>

namespace NeoEngine::Networking {

enum class EntityState : uint8_t { Free, PendingSpawn, Spawned, PendingDestroy };
struct NetworkEntityRecord { uint32_t networkId{}; uint32_t ownerId{}; uint64_t spawnTick{}; uint64_t revision{}; EntityState state{EntityState::Free}; };

class NetworkEntityLifecycle {
public:
    static constexpr uint16_t Capacity=4096;
    bool spawn(uint32_t id,uint32_t owner,uint64_t tick){if(!id||!owner||find(id))return false;for(auto&e:entities_)if(e.state==EntityState::Free){e={id,owner,tick,0,EntityState::PendingSpawn};return true;}return false;}
    bool confirmSpawn(uint32_t id){auto*e=find(id);if(!e||e->state!=EntityState::PendingSpawn)return false;e->state=EntityState::Spawned;return true;}
    bool destroy(uint32_t id){auto*e=find(id);if(!e||e->state!=EntityState::Spawned)return false;e->state=EntityState::PendingDestroy;return true;}
    bool confirmDestroy(uint32_t id){auto*e=find(id);if(!e||e->state!=EntityState::PendingDestroy)return false;*e={};return true;}
    bool bumpRevision(uint32_t id){auto*e=find(id);if(!e||e->state!=EntityState::Spawned)return false;++e->revision;return true;}
    const NetworkEntityRecord* get(uint32_t id)const{for(auto const&e:entities_)if(e.networkId==id&&e.state!=EntityState::Free)return &e;return nullptr;}
private: NetworkEntityRecord* find(uint32_t id){for(auto&e:entities_)if(e.networkId==id&&e.state!=EntityState::Free)return &e;return nullptr;} std::array<NetworkEntityRecord,Capacity> entities_{};
};
}
