#pragma once

#include "ISystem.h"
#include "System.h"
#include "Registry.h"
#include <memory>

namespace NeoEngine
{

// Wrapper: bungkus System lama (butuh Registry) jadi ISystem baru
// Sehingga bisa masuk SystemManager tanpa ubah MovementSystem dll
class RegistrySystem : public ISystem
{
public:
    RegistrySystem(std::unique_ptr<System> system, Registry& registry)
        : inner(std::move(system))
        , registry(registry)
    {}

    void Update() override
    {
        inner->Update(0.0f, registry);
    }

    void Update(float dt) override
    {
        inner->Update(dt, registry);
    }

private:
    std::unique_ptr<System> inner;
    Registry&               registry;
};

} // namespace NeoEngine
