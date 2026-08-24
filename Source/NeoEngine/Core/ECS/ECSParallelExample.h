#pragma once
#include "JobSystem.h"
#include "Archetype.h"
#include "MoveSystem.h"

namespace NeoEngine
{
    inline void RunECSParallel(JobSystem& jobSystem, Archetype& archetype)
    {
        jobSystem.Dispatch(archetype.GetEntityCount(), [&](size_t i)
        {
            // Panggil MoveSystem (Pastikan MoveSystem juga inline atau terpanggil benar)
            MoveSystem(archetype); 
        });
    }
}
