#pragma once
#include "Registry.h"
#include <unordered_set>

class ReactiveSystem : public System {
protected:
    std::unordered_set<Entity> entities;
    Signature requiredSignature;
public:
    ReactiveSystem(const Signature& sig) : requiredSignature(sig) {}
    virtual ~ReactiveSystem() = default;

    virtual void UpdateEntityMembership(Registry& registry, Entity entity) override {
        if (!registry.IsAlive(entity)) {
            entities.erase(entity);
            return;
        }
        const Signature sig = registry.GetSignature(entity);
        if ((sig & requiredSignature) == requiredSignature) {
            entities.insert(entity);
        } else {
            entities.erase(entity);
        }
    }

    template<typename Func> void Each(Registry& registry, Func func) {
        for (auto e : entities) { func(e); }
    }
};
