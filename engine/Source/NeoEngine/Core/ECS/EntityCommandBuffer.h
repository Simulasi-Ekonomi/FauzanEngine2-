#pragma once

#include <vector>
#include <functional>
#include "Registry.h"

namespace NeoEngine {

class EntityCommandBuffer {

private:

    std::vector<std::function<void(Registry&)>> commands;

public:

    template<typename T>
    void AddComponent(Entity e, const T& component)
    {
        commands.emplace_back([=](Registry& registry)
        {
            registry.AddComponent<T>(e, component);
        });
    }

    template<typename T>
    void RemoveComponent(Entity e)
    {
        commands.emplace_back([=](Registry& registry)
        {
            registry.RemoveComponent<T>(e);
        });
    }

    void CreateEntity(std::function<void(Entity)> initializer)
    {
        commands.emplace_back([=](Registry& registry)
        {
            Entity e = registry.CreateEntity();
            initializer(e, registry);
        });
    }

    void Playback(Registry& registry)
    {
        for(auto& cmd : commands)
        {
            cmd(registry);
        }

        commands.clear();
    }

};

}
