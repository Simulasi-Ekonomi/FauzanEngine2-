#pragma once
#include "System.h"

namespace NeoEngine
{

inline void MoveSystem(Archetype& arch)
{
    Neo::ForEach(arch, [&](size_t i)
    {
        // update entity position
    });
}

}
