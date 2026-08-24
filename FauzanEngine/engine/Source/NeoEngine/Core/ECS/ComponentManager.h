#pragma once

#include <unordered_map>
#include "Entity.h"

namespace NeoEngine
{

template<typename Component>
class ComponentManager
{
public:
    void AddComponent(const Entity& e, const Component& c)
    {
        components[e.id] = c;
    }

    void RemoveComponent(const Entity& e)
    {
        components.erase(e.id);
    }

    bool HasComponent(const Entity& e) const
    {
        return components.count(e.id) > 0;
    }

    Component& GetComponent(const Entity& e)
    {
        return components.at(e.id);
    }

private:
    std::unordered_map<uint32_t, Component> components;
};

} // namespace NeoEngine
