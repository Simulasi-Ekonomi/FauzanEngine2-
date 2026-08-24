#include "SubsystemManager.h"
#include "Subsystem.h"

namespace NeoEngine
{

std::vector<Subsystem*> SubsystemManager::s_Systems;

void SubsystemManager::InitAll()
{
    for (auto* sys : s_Systems)
    {
        if (sys) sys->Init();
    }
}

void SubsystemManager::TickAll()
{
    for (auto* sys : s_Systems)
    {
        if (sys) sys->Tick();
    }
}

void SubsystemManager::ShutdownAll()
{
    for (auto* sys : s_Systems)
    {
        if (sys) sys->Shutdown();
    }
}

bool SubsystemManager::AreAllInitialized()
{
    for (auto* sys : s_Systems)
    {
        if (!sys || !sys->IsInitialized())
            return false;
    }
    return true;
}

}
