#pragma once
#include "System.h"

namespace Neo
{

inline void MoveSystem(Archetype& arch)
{
    Neo::ForEach(arch, [&](size_t i)
    {
        // update entity position
    });
}

}
