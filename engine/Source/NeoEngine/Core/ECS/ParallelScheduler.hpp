#pragma once

#include "Registry.h"
#include "../Threading/JobSystem.h"

class ParallelScheduler
{

private:

    JobSystem jobs;

public:

    void RunSystems(std::vector<std::unique_ptr<System>>& systems,
                    Registry& registry,
                    float dt)
    {

        for (auto& sys : systems)
        {

            jobs.Dispatch([&, s=sys.get()]
            {
                s->Update(registry, dt);
            });

        }

    }

};
