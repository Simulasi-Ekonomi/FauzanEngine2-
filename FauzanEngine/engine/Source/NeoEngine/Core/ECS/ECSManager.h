#pragma once
#include "Entity.h"
#include "Component.h"
#include "System.h"
#include <unordered_map>
#include <vector>
#include <mutex>

namespace NeoEngine {

class ECSManager {
public:
    Entity CreateEntity() {
        std::lock_guard<std::mutex> lock(mutex);
        Entity e{nextID++};
        entities.push_back(e);
        return e;
    }

    template<typename T>
    void AddComponent(Entity e, std::shared_ptr<T> comp) {
        std::lock_guard<std::mutex> lock(mutex);
        components[e.id][std::type_index(typeid(T))] = comp;
    }

    template<typename T>
    std::shared_ptr<T> GetComponent(Entity e) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it1 = components.find(e.id);
        if (it1 == components.end()) return nullptr;
        auto it2 = it1->second.find(std::type_index(typeid(T)));
        if (it2 == it1->second.end()) return nullptr;
        return std::static_pointer_cast<T>(it2->second);
    }

    void RegisterSystem(std::shared_ptr<System> system) {
        std::lock_guard<std::mutex> lock(mutex);
        systems.push_back(system);
    }

    void UpdateSystems(float dt) {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& s : systems) {
            s->Update(dt);
        }
    }

private:
    EntityID nextID = 1;
    [[maybe_unused]] std::vector<Entity> entities;
    std::unordered_map<EntityID, std::unordered_map<ComponentType, ComponentPtr>> components;
    [[maybe_unused]] std::vector<std::shared_ptr<System>> systems;
    std::mutex mutex;
};

} // namespace NeoEngine
