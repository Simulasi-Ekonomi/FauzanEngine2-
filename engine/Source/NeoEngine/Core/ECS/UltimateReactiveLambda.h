#pragma once
#include "Registry.h"
#include "ReactiveSystem.h"
#include <unordered_map>
#include <functional>
#include <vector>

// ============================================================
// LiveEditorLambda: reactive + chunked + lambda ECS
class LiveEditorLambda {
private:
    Registry& registry;

    struct LambdaData {
        Entity entity;
        std::function<void(Entity,Registry&,float)> lambda;
    };
    [[maybe_unused]] std::vector<LambdaData> lambdas;

public:
    LiveEditorLambda(Registry& reg) : registry(reg) {}

    // Register chunked/reactive system
    template<typename SystemType, typename... Args>
    SystemType* RegisterLambdaSystem(Args&&... args) {
        auto* sys = registry.RegisterSystem<SystemType>(std::forward<Args>(args)...);
        return sys;
    }

    // Run lambda once entity sudah valid
    void RunLambda(Entity e, std::function<void(Entity,Registry&,float)> lambda) {
        if(!registry.IsAlive(e)) return;
        lambdas.push_back({e, lambda});
    }

    // Update semua lambdas setiap frame
    void UpdateSystems(float dt) {
        for(auto& ld : lambdas) {
            if(!registry.IsAlive(ld.entity)) continue;
            ld.lambda(ld.entity, registry, dt);
        }
    }
};
