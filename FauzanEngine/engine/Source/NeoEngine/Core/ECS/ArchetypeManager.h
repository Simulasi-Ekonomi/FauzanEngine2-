#pragma once
#include <unordered_map>
#include <vector>
#include "Entity.h"

namespace NeoEngine {

class ArchetypeManager {
public:
    void RegisterEntity(const Entity& e) {
        archetypes[mask].push_back(e);
    }

    const std::vector<Entity>& GetArchetype(uint64_t mask) const {
        return archetypes.at(mask);
    }

private:
    std::unordered_map<uint64_t, std::vector<Entity>> archetypes;
};

}