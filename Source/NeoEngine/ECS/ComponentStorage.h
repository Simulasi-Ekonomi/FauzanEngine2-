#pragma once
#include <unordered_map>
#include <vector>
#include "Entity.h"

template<typename T>
class ComponentStorage {
public:
    void Add(EntityID entity, const T& component) {
        Data[entity] = component;
    }
    
    void Remove(EntityID entity) {
        Data.erase(entity);
    }
    
    T* Get(EntityID entity) {
        auto it = Data.find(entity);
        return it != Data.end() ? &it->second : nullptr;
    }
    
    std::unordered_map<EntityID, T>& GetAll() {
        return Data;
    }

private:
    std::unordered_map<EntityID, T> Data;
};
