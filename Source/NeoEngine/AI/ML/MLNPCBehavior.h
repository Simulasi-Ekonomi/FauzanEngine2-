#pragma once
#include <vector>

struct NPCState
{
    float x;
    float y;
    float goal;
};

class MLNPCBehavior
{
public:

    void AddNPC(const NPCState& npc);
    void Update();

private:

    std::vector<NPCState> npcs;
};
