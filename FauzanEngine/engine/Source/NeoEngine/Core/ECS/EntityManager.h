#pragma once

#include <vector>
#include <cstdint>
#include <unordered_map>
#include <any>
#include <typeindex>
#include "Entity.h"

namespace NeoEngine
{

class EntityManager
{
public:
    Entity CreateEntity()
    {
        return Entity{nextID++};
    }

    void DestroyEntity(const Entity& e)
    {
        components.erase(e.id);
    }

    bool IsAlive(const Entity& e) const
    {
        return components.count(e.id) > 0;
    }

    template<typename T, typename... Args>
    T& AddComponent(Entity e, Args&&... args)
    {
        auto key = std::type_index(typeid(T));
        components[e.id][key] = T{std::forward<Args>(args)...};
        return std::any_cast<T&>(components[e.id][key]);
    }

    template<typename T>
    T& GetComponent(Entity e)
    {
        auto key = std::type_index(typeid(T));
        return std::any_cast<T&>(components[e.id][key]);
    }

    template<typename T>
    bool HasComponent(Entity e) const
    {
        auto key = std::type_index(typeid(T));
        auto it  = components.find(e.id);
        if (it == components.end()) return false;
        return it->second.count(key) > 0;
    }

    template<typename T1, typename T2>
    std::vector<Entity> GetEntitiesWith()
    {
        std::vector<Entity> result;
        auto k1 = std::type_index(typeid(T1));
        auto k2 = std::type_index(typeid(T2));
        for (auto& [id, comps] : components)
            if (comps.count(k1) && comps.count(k2))
                result.push_back(Entity{id});
        return result;
    }

private:
    uint32_t nextID = 1;
    std::unordered_map<
        uint32_t,
        std::unordered_map<std::type_index, std::any>
    > components;
};

} // namespace NeoEngine
