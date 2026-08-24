#pragma once
#include "Archetype.h"

namespace NeoEngine {

template<typename Func>
void IterateArchetype(Archetype* archetype, Func func)
{
    for (size_t i = 0; i < archetype->entities.size(); ++i)
    {
        func(archetype->entities[i]);
    }
}

}
