#pragma once
#include <vector>
#include <string>
#include <functional>

namespace NeoEngine {
struct Pet { std::string id,name,species; int level=1,exp=0; int hp=50,attack=10,defense=5; int happiness=100; int evolveLevel=10; bool canEvolve=false; };
class PetSystem {
private:
    std::vector<Pet> m_Pets; int m_ActivePet=-1; std::function<void(Pet&)> m_OnEvolve;
public:
    Pet* AdoptPet(const std::string& name,const std::string& species){ m_Pets.push_back({name,name,species}); return &m_Pets.back(); }
    void FeedPet(int index,int food){ if(index<0||index>=m_Pets.size())return; auto& p=m_Pets[index]; p.happiness+=food*5; if(p.happiness>100)p.happiness=100; p.exp+=food*2; }
    void TrainPet(int index,int sessions){ if(index<0||index>=m_Pets.size())return; auto& p=m_Pets[index]; p.attack+=sessions*2; p.defense+=sessions; p.exp+=sessions*5; CheckEvolve(p); }
    void CheckEvolve(Pet& p){ if(p.exp>=p.evolveLevel*50&&p.canEvolve){ p.level++; p.hp+=20; p.attack+=5; p.defense+=3; p.evolveLevel+=5; p.exp=0; if(m_OnEvolve)m_OnEvolve(p); } }
    void SetActivePet(int index){ if(index>=0&&index<m_Pets.size())m_ActivePet=index; }
    Pet* GetActivePet(){ return m_ActivePet>=0?&m_Pets[m_ActivePet]:nullptr; }
    const std::vector<Pet>& GetPets()const{ return m_Pets; }
    void SetOnEvolve(std::function<void(Pet&)> cb){ m_OnEvolve=cb; }
};
}
