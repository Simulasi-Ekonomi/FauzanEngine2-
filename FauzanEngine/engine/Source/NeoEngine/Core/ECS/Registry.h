#pragma once

#include <unordered_map>
#include <memory>
#include <typeindex>
#include <vector>
#include <cstdint>

#include "Entity.h"
#include "ComponentStorage.h"

namespace NeoEngine {

// INDUSTRY AAA CORE ECS REGISTRY (ENHANCED, NON-BREAKING)
class Registry {
private:
    std::unordered_map<std::type_index,
        std::shared_ptr<IComponentStorage>> componentPools;

    std::vector<Entity> entities;

    // FIX: gunakan ID terpisah (AAA pattern)
    uint32_t nextEntityID = 1;

public:

    // =========================
    // ENTITY MANAGEMENT
    // =========================
    Entity CreateEntity() {
        Entity e{};
        e.id = nextEntityID++;     // FIX: tidak increment Entity
        entities.push_back(e);
        return e;
    }

    const std::vector<Entity>& GetEntities() const {
        return entities;
    }

    // =========================
    // COMPONENT MANAGEMENT
    // =========================
    template<typename T>
    ComponentStorage<T>& GetPool() {
        std::type_index type = std::type_index(typeid(T));

        if (componentPools.find(type) == componentPools.end()) {
            componentPools[type] = std::make_shared<ComponentStorage<T>>();
        }

        return *static_cast<ComponentStorage<T>*>(componentPools[type].get());
    }

    template<typename T>
    void AddComponent(Entity e, const T& component) {
        GetPool<T>().Add(e, component);
    }

    template<typename T>
    T& GetComponent(Entity e) {
        return GetPool<T>().Get(e);
    }

    template<typename T>
    bool HasComponent(Entity e) {
        return GetPool<T>().Has(e);
    }

    // =========================
    // SIMPLE VIEW (AAA BASIC)
    // =========================
    template<typename... Components>
    std::vector<Entity> ViewEntities() {
        std::vector<Entity> result;

        for (auto e : entities) {
            if ((HasComponent<Components>(e) && ...)) {
                result.push_back(e);
            }
        }

        return result;
    }
};

}
