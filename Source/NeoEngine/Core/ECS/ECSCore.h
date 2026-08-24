#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <type_traits>
#include <memory>
#include <algorithm>

namespace NeoEngine {

using EntityID = uint32_t;

// Komponen dasar
struct Position { float x = 0, y = 0, z = 0; };
struct Velocity { float vx = 0, vy = 0, vz = 0; };

// Pool komponen generik
class IComponentPool {
public:
    virtual ~IComponentPool() = default;
};

template<typename T>
class ComponentPool : public IComponentPool {
public:
    T& Add(EntityID id, T comp = {}) {
        components[id] = comp;
        return components[id];
    }
    void Remove(EntityID id) { components.erase(id); }
    T* Get(EntityID id) {
        auto it = components.find(id);
        return it != components.end() ? &it->second : nullptr;
    }
    bool Has(EntityID id) const { return components.find(id) != components.end(); }
    const std::unordered_map<EntityID, T>& GetAll() const { return components; }
private:
    std::unordered_map<EntityID, T> components;
};

// Registry ECS produksi
class Registry {
public:
    // Registrasi tipe komponen
    template<typename T>
    void RegisterComponent() {
        std::string name(typeid(T).name());
        if (m_Pools.find(name) == m_Pools.end()) {
            m_Pools[name] = std::make_unique<ComponentPool<T>>();
        }
    }

    // Menambahkan komponen ke entity
    template<typename T>
    T& AddComponent(EntityID id, T comp = {}) {
        std::string name(typeid(T).name());
        auto& pool = GetPool<T>(name);
        return pool.Add(id, comp);
    }

    // Hapus komponen
    template<typename T>
    void RemoveComponent(EntityID id) {
        std::string name(typeid(T).name());
        auto it = m_Pools.find(name);
        if (it != m_Pools.end()) {
            static_cast<ComponentPool<T>*>(it->second.get())->Remove(id);
        }
    }

    // Ambil komponen
    template<typename T>
    T& GetComponent(EntityID id) {
        std::string name(typeid(T).name());
        auto& pool = GetPool<T>(name);
        T* ptr = pool.Get(id);
        // Untuk kesederhanaan, diasumsikan komponen ada
        return *ptr;
    }

    // Cek apakah entity memiliki semua komponen yang diminta
    template<typename... Components>
    std::vector<EntityID> ViewEntities() {
        std::vector<EntityID> result;
        if constexpr (sizeof...(Components) == 0) return result;

        // Ambil pool komponen pertama
        using FirstType = std::tuple_element_t<0, std::tuple<Components...>>;
        std::string firstName(typeid(FirstType).name());
        auto it = m_Pools.find(firstName);
        if (it == m_Pools.end()) return result;
        auto* firstPool = static_cast<ComponentPool<FirstType>*>(it->second.get());
        
        for (auto& [id, comp] : firstPool->GetAll()) {
            if (HasAllComponents<Components...>(id)) {
                result.push_back(id);
            }
        }
        return result;
    }

private:
    std::unordered_map<std::string, std::unique_ptr<IComponentPool>> m_Pools;

    template<typename T>
    ComponentPool<T>& GetPool(const std::string& name) {
        auto it = m_Pools.find(name);
        if (it == m_Pools.end()) {
            RegisterComponent<T>();
            return *static_cast<ComponentPool<T>*>(m_Pools[name].get());
        }
        return *static_cast<ComponentPool<T>*>(it->second.get());
    }

    template<typename First, typename... Rest>
    bool HasAllComponents(EntityID id) {
        std::string name(typeid(First).name());
        auto it = m_Pools.find(name);
        if (it == m_Pools.end()) return false;
        auto* pool = static_cast<ComponentPool<First>*>(it->second.get());
        if (!pool->Has(id)) return false;
        if constexpr (sizeof...(Rest) > 0)
            return HasAllComponents<Rest...>(id);
        else
            return true;
    }
};

} // namespace NeoEngine
