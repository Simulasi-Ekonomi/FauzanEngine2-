#include "MLNPCBehavior.h"

void MLNPCBehavior::AddNPC(const NPCState& npc)
{
    npcs.push_back(npc);
}

void MLNPCBehavior::Update()
{
    for(auto& n : npcs)
    {
        n.x += (n.goal - n.x) * 0.1f;
    }
}
