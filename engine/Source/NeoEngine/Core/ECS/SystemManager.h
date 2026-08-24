#pragma once

#include <vector>
#include <memory>
#include "ISystem.h"

namespace NeoEngine
{

class SystemManager
{
public:

    void addSystem(std::unique_ptr<ISystem> system)
    {
        systems.push_back(std::move(system));
    }

    template<typename T, typename... Args>
    T* AddSystem(Args&&... args)
    {
        auto s   = std::make_unique<T>(std::forward<Args>(args)...);
        T*   ptr = s.get();
        systems.push_back(std::move(s));
        return ptr;
    }

    void update()
    {
        for (auto& s : systems)
            s->Update();
    }

    void update(float dt)
    {
        for (auto& s : systems)
            s->Update(dt);
    }

    void clear()
    {
        systems.clear();
    }

private:
    std::vector<std::unique_ptr<ISystem>> systems;
};

} // namespace NeoEngine
