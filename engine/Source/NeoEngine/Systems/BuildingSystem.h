#pragma once
#include <string>
#include <unordered_map>
namespace NeoEngine {
struct Building{std::string id,type,name;float posX,posY,posZ,rotX,rotY,rotZ;int level=1,cost=100,incomePerMinute=0;bool completed=true;};
class BuildingSystem {
    std::unordered_map<std::string,Building> m_Buildings;
    int m_NextId=1,m_TotalIncome=0;
    float m_TimeAccum=0;
public:
    std::string PlaceBuilding(const std::string& type,float x,float y,float z){std::string id="bld_"+std::to_string(m_NextId++);Building b{id,type,type,x,y,z};if(type=="house"){b.cost=100;b.incomePerMinute=5;}else if(type=="shop"){b.cost=500;b.incomePerMinute=30;}else if(type=="factory"){b.cost=2000;b.incomePerMinute=100;}m_Buildings[id]=b;return id;}
    bool UpgradeBuilding(const std::string& id){auto it=m_Buildings.find(id);if(it==m_Buildings.end())return false;it->second.level++;it->second.incomePerMinute*=2;return true;}
    void Update(float dt){m_TimeAccum+=dt;if(m_TimeAccum>=60.f){m_TimeAccum-=60.f;for(auto&[id,b]:m_Buildings)if(b.completed)m_TotalIncome+=b.incomePerMinute;}}
    int GetTotalIncome()const{return m_TotalIncome;}
};
}
