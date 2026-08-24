#pragma once
#include <vector>
#include <cstdint>

namespace NeoEngine {

using EntityID = uint32_t;
constexpr EntityID V4_INVALID_ENTITY = 0;
constexpr uint32_t V4_MAX_ENTITIES = 100000;

struct PositionComponent { std::vector<float> x, y, z; };
struct VelocityComponent { std::vector<float> vx, vy, vz; };
struct ModelComponent { std::vector<uint32_t> meshID; };

class FauzanEntityManager {
public:
    FauzanEntityManager() : m_FreeList{} {
        for (uint32_t i = 0; i < V4_MAX_ENTITIES; ++i) m_Alive[i] = false;
        for (uint32_t i = 1; i < V4_MAX_ENTITIES; ++i) m_FreeList.push_back(i);
    }

    EntityID CreateEntity() {
        if (m_FreeList.empty()) return V4_INVALID_ENTITY;
        EntityID id = m_FreeList.back();
        m_FreeList.pop_back();
        m_Alive[id] = true;
        // resize komponen otomatis
        if (id >= positions.x.size()) {
            positions.x.resize(id + 1);
            positions.y.resize(id + 1);
            positions.z.resize(id + 1);
            velocities.vx.resize(id + 1);
            velocities.vy.resize(id + 1);
            velocities.vz.resize(id + 1);
            models.meshID.resize(id + 1);
        }
        return id;
    }

    void DestroyEntity(EntityID id) {
        if (id < V4_MAX_ENTITIES && m_Alive[id]) {
            m_Alive[id] = false;
            m_FreeList.push_back(id);
        }
    }

    bool IsAlive(EntityID id) const { return id < V4_MAX_ENTITIES && m_Alive[id]; }

    PositionComponent positions;
    VelocityComponent velocities;
    ModelComponent models;

    template<typename Func>
    void ForEach(Func&& func) {
        for (EntityID id = 0; id < V4_MAX_ENTITIES; ++id)
            if (m_Alive[id]) func(id);
    }

    template<typename... Components>
    std::vector<EntityID> Query() {
        std::vector<EntityID> result;
        ForEach([&](EntityID id) {
            if ((HasComponent<Components>(id) && ...)) result.push_back(id);
        });
        return result;
    }

private:
    bool m_Alive[V4_MAX_ENTITIES];
    std::vector<EntityID> m_FreeList;

    template<typename T> bool HasComponent(EntityID) const { return false; }
};

template<> inline bool FauzanEntityManager::HasComponent<PositionComponent>(EntityID id) const { return id < positions.x.size(); }
template<> inline bool FauzanEntityManager::HasComponent<VelocityComponent>(EntityID id) const { return id < velocities.vx.size(); }
template<> inline bool FauzanEntityManager::HasComponent<ModelComponent>(EntityID id) const { return id < models.meshID.size(); }
}
