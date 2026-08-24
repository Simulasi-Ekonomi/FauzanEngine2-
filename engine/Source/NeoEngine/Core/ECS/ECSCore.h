#pragma once
#include <vector>
#include <unordered_map>
#include <memory>
#include <cassert>
#include <typeindex>

namespace NeoEngine {

using EntityID = uint32_t;
static constexpr EntityID INVALID_ENTITY = 0xFFFFFFFF;

class BaseComponentPool {
public:
    virtual ~BaseComponentPool() = default;
    virtual void Remove(EntityID id) = 0;
};

// Sparse Set Component Pool - Standar Senior
template<typename T>
class ComponentPool : public BaseComponentPool {
public:
    std::vector<T> components;     // Packed data (Cache Friendly)
    std::vector<EntityID> entities; // Map back to entity
    std::unordered_map<EntityID, size_t> sparse; // EntityID -> Index di packed array

    T* Add(EntityID id, T component) {
        if (sparse.find(id) != sparse.end()) return &components[sparse[id]];
        sparse[id] = components.size();
        components.push_back(component);
        entities.push_back(id);
        return &components.back();
    }

    void Remove(EntityID id) override {
        if (sparse.find(id) == sparse.end()) return;
        size_t index = sparse[id];
        // Swap with last element for O(1) removal
        components[index] = components.back();
        entities[index] = entities.back();
        sparse[entities[index]] = index;
        
        components.pop_back();
        entities.pop_back();
        sparse.erase(id);
    }

    T* Get(EntityID id) {
        return (sparse.find(id) != sparse.end()) ? &components[sparse[id]] : nullptr;
    }
};

class Registry {
public:
    EntityID CreateEntity() {
        return nextID++;
    }

    template<typename T, typename... Args>
    T* Emplace(EntityID id, Args&&... args) {
        return GetPool<T>().Add(id, T{std::forward<Args>(args)...});
    }

    template<typename T>
    T* Get(EntityID id) { return GetPool<T>().Get(id); }

private:
    EntityID nextID = 0;
    std::unordered_map<std::type_index, std::unique_ptr<BaseComponentPool>> pools;

    template<typename T>
    ComponentPool<T>& GetPool() {
        auto idx = std::type_index(typeid(T));
        if (!pools[idx]) pools[idx] = std::make_unique<ComponentPool<T>>();
        return *static_cast<ComponentPool<T>*>(pools[idx].get());
    }
};

} // namespace
