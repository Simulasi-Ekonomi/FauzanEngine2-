#pragma once
#include "Archetype.h"
#include <unordered_map>
#include <memory>

namespace NeoEngine {

class ArchetypeRegistry {

private:

    std::unordered_map<size_t, std::unique_ptr<Archetype>> archetypes;

public:

    Archetype* GetOrCreate(const Signature& sig)
    {
        size_t hash = sig.to_ullong();

        auto it = archetypes.find(hash);

        if (it != archetypes.end())
            return it->second.get();

        archetypes[hash] = std::make_unique<Archetype>(sig);

        return archetypes[hash].get();
    }

};

}
