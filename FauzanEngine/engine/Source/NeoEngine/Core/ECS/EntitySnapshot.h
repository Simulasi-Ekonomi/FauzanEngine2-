#pragma once

#include <vector>
#include <cstdint>
#include "Entity.h"

struct Snapshot
{
    uint64_t tick;
    [[maybe_unused]] std::vector<Entity> entities;
};

class SnapshotSystem
{

private:

    [[maybe_unused]] std::vector<Snapshot> history;

public:

    void Capture(uint64_t tick,const std::vector<Entity>& ents)
    {

        Snapshot s;

        s.tick = tick;
        s.entities = ents;

        history.push_back(s);

    }

    const Snapshot* Get(uint64_t tick) const
    {

        for (const auto& s : history)
        {
            if (s.tick == tick)
                return &s;
        }

        return nullptr;

    }

};
