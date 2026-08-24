#pragma once
#include <vector>
#include <string>
#include <unordered_map>

namespace NeoEngine {

struct Resource { std::string type; int count=0; };

class SurvivalCraftSystem {
private:
    std::unordered_map<std::string, int> m_Resources;
    int m_Health=100, m_Hunger=100, m_Thirst=100, m_Oxygen=100;
    float m_TimeAccum=0;
    
public:
    SurvivalCraftSystem(){ m_Resources["wood"]=0; m_Resources["stone"]=0; m_Resources["iron"]=0; m_Resources["food"]=0; m_Resources["water"]=0; }
    void GatherResource(const std::string& type, int amount){ m_Resources[type] += amount; }
    int GetResource(const std::string& type) const { auto it=m_Resources.find(type); return it!=m_Resources.end()?it->second:0; }
    bool ConsumeResource(const std::string& type, int amount){ if(GetResource(type)<amount)return false; m_Resources[type]-=amount; return true; }
    
    bool CraftItem(const std::string& item){
        if(item=="axe"){ if(ConsumeResource("wood",3)&&ConsumeResource("stone",2))return true; }
        else if(item=="pickaxe"){ if(ConsumeResource("wood",2)&&ConsumeResource("stone",3))return true; }
        else if(item=="sword"){ if(ConsumeResource("wood",1)&&ConsumeResource("iron",3))return true; }
        else if(item=="campfire"){ if(ConsumeResource("wood",5)&&ConsumeResource("stone",1))return true; }
        return false;
    }
    
    void Update(float dt){
        m_TimeAccum += dt;
        if(m_TimeAccum >= 30.0f){ m_TimeAccum -= 30.0f; m_Hunger -= 2; m_Thirst -= 3; if(m_Hunger<0)m_Hunger=0; if(m_Thirst<0)m_Thirst=0; if(m_Hunger<=0||m_Thirst<=0)m_Health-=5; }
    }
    
    void Eat(int amount=10){ m_Hunger += amount; if(m_Hunger>100)m_Hunger=100; m_Health += 5; if(m_Health>100)m_Health=100; }
    void Drink(int amount=10){ m_Thirst += amount; if(m_Thirst>100)m_Thirst=100; }
    int GetHealth() const { return m_Health; }
    int GetHunger() const { return m_Hunger; }
    int GetThirst() const { return m_Thirst; }
};

} // namespace NeoEngine
