#pragma once
#include <vector>
#include <unordered_map>
#include <typeindex>
#include "IComponentStorage.h"
#include "Entity.h"

namespace NeoEngine {

// INDUSTRY AAA CORE
template<typename T>
class ComponentStorage : public IComponentStorage {
private:
    std::unordered_map<Entity, T> components;

public:
    void Add(Entity e, const T& component) {
        components[e] = component;
    }

    T& Get(Entity e) {
        return components.at(e);
    }

    bool Has(Entity e) const {
        return components.find(e) != components.end();
    }

    void Remove(Entity e) {
        components.erase(e);
    }
};

}
