#pragma once
#include <string>
#include <vector>
#include <cmath>
namespace NeoEngine {
enum class CropType{Wheat,Corn,Tomato,Carrot,Potato,Strawberry};
enum class GrowthStage{Seed,Sprout,Growing,Flowering,Harvestable};
struct Crop{CropType type;GrowthStage stage=GrowthStage::Seed;float growthProgress=0,growthTime=300;bool watered=false;int harvestCount=1;};
struct FarmPlot{int x,z;Crop* crop=nullptr;bool tilled=false;};
class FarmingSystem {
    const int FARM_SIZE=8;
    std::vector<FarmPlot> m_Plots;
    int m_Money=100;
public:
    FarmingSystem(){m_Plots.resize(FARM_SIZE*FARM_SIZE);}
    bool TillPlot(int x,int z){if(x<0||x>=FARM_SIZE||z<0||z>=FARM_SIZE)return false;m_Plots[z*FARM_SIZE+x].tilled=true;return true;}
    bool PlantCrop(int x,int z,CropType type){if(x<0||x>=FARM_SIZE||z<0||z>=FARM_SIZE)return false;auto& p=m_Plots[z*FARM_SIZE+x];if(!p.tilled||p.crop)return false;p.crop=new Crop{type};return true;}
    bool WaterPlot(int x,int z){if(x<0||x>=FARM_SIZE||z<0||z>=FARM_SIZE)return false;auto& p=m_Plots[z*FARM_SIZE+x];if(!p.crop)return false;p.crop->watered=true;return true;}
    int HarvestPlot(int x,int z){if(x<0||x>=FARM_SIZE||z<0||z>=FARM_SIZE)return 0;auto& p=m_Plots[z*FARM_SIZE+x];if(!p.crop||p.crop->stage!=GrowthStage::Harvestable)return 0;int c=p.crop->harvestCount;delete p.crop;p.crop=nullptr;p.tilled=false;m_Money+=c*10;return c;}
    void Update(float dt){for(auto& p:m_Plots){if(!p.crop)continue;auto& c=*p.crop;c.growthProgress+=dt*(c.watered?2.f:0.5f);if(c.growthProgress>=c.growthTime){c.growthProgress=c.growthTime;c.stage=GrowthStage::Harvestable;}}}
    int GetMoney()const{return m_Money;}
    void AddMoney(int a){m_Money+=a;}
};
}
