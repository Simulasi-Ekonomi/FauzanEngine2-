#pragma once
#include <vector>
#include <queue>
#include <cstdint>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

using EntityID = uint32_t;
constexpr EntityID MAX_ENTITIES = 1000000;
constexpr EntityID INVALID_ENTITY = 0;

class EntityManager {
private:
    std::vector<EntityID> m_Entities;
    std::queue<EntityID> m_FreeList;
    std::vector<uint32_t> m_Generations;
    std::vector<bool> m_Active;
    size_t m_LivingEntityCount = 0;
    std::function<void(EntityID, EntityID)> m_OnCollision;

public:
    EntityManager() {
        m_Entities.reserve(MAX_ENTITIES);
        m_Generations.resize(MAX_ENTITIES, 0);
        m_Active.resize(MAX_ENTITIES, false);
        for (EntityID i = 1; i < MAX_ENTITIES; ++i) m_FreeList.push(i);
    }

    EntityID CreateEntity() {
        if (m_FreeList.empty()) return INVALID_ENTITY;
        EntityID id = m_FreeList.front();
        m_FreeList.pop();
        m_Active[id] = true;
        m_Generations[id]++;
        m_LivingEntityCount++;
        if (id >= m_Entities.size()) m_Entities.resize(id + 1);
        return id;
    }

    void DestroyEntity(EntityID id) {
        if (id == 0 || id >= MAX_ENTITIES || !m_Active[id]) return;
        m_Active[id] = false;
        m_FreeList.push(id);
        m_LivingEntityCount--;
    }

    bool IsAlive(EntityID id) const { return id < MAX_ENTITIES && m_Active[id]; }
    uint32_t GetGeneration(EntityID id) const { return (id < MAX_ENTITIES) ? m_Generations[id] : 0; }
    size_t GetLivingEntityCount() const { return m_LivingEntityCount; }

    template<typename Func> void ForEach(Func&& func) {
        for (EntityID id = 1; id < m_Active.size(); ++id) if (m_Active[id]) func(id);
    }

    void TriggerCollision(EntityID a, EntityID b) { if (m_OnCollision) m_OnCollision(a, b); }
    void SetOnCollision(std::function<void(EntityID, EntityID)> cb) { m_OnCollision = cb; }
};

}
