#pragma once
#include "EntityManager.h"
#include "SystemManager.h"
namespace NeoEngine {
class World {
    EntityManager m_EntityManager; SystemManager m_SystemManager; float m_Time=0; bool m_Running=false;
public:
    EntityManager& GetEntityManager(){return m_EntityManager;}
    SystemManager& GetSystemManager(){return m_SystemManager;}
    void Update(float dt){m_Time+=dt;if(m_Running)m_SystemManager.UpdateAll(dt,m_EntityManager);}
    void Start(){m_Running=true;} void Stop(){m_Running=false;}
    float GetTime()const{return m_Time;}
};
}
