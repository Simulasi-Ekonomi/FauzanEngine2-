#pragma once
#include <vector>
#include <string>
#include <cstdlib>
#include <functional>

namespace NeoEngine {
struct Fish { std::string name; float weight; int rarity; int value; };
struct FishingRod { std::string name; int power=1; int luck=0; int durability=100; };
class FishingSystem {
private:
    FishingRod m_Rod{"Basic Rod",1,0,100}; std::vector<Fish> m_Caught;
    std::function<void(const Fish&)> m_OnCatch;
    std::vector<Fish> m_FishDB={{"Carp",1,1,5},{"Trout",2,2,10},{"Bass",3,2,15},{"Salmon",5,3,30},{"Tuna",10,3,50},{"Golden Fish",2,5,200},{"Shark",50,4,100},{"Whale",200,5,500}};
public:
    Fish CastLine(){ int r=rand()%100; int luck=m_Rod.luck; Fish* f=nullptr; float bestRarity=0;
        for(auto& fish:m_FishDB){ float chance=(10.0f-fish.rarity*2+luck)/10.0f; if(chance>1)chance=1; if(r<(int)(chance*100)&&fish.rarity>bestRarity){ f=&fish; bestRarity=fish.rarity; } }
        if(f){ Fish caught=*f; caught.weight+=(float)(rand()%100)/10.0f; m_Caught.push_back(caught); m_Rod.durability--; if(m_OnCatch)m_OnCatch(caught); return caught; }
        return {"Nothing",0,0,0};
    }
    bool UpgradeRod(){ if(m_Rod.power<5){ m_Rod.power++; m_Rod.luck++; return true; } return false; }
    void RepairRod(){ m_Rod.durability=100; }
    int GetDurability()const{ return m_Rod.durability; }
    const auto& GetCaught()const{ return m_Caught; }
    void SetOnCatch(std::function<void(const Fish&)> cb){ m_OnCatch=cb; }
};
}
