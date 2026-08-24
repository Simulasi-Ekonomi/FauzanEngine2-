#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace NeoEngine {
struct Furniture { std::string id,name,type; int x,y,rot; int cost=0; };
struct House { std::string ownerId; std::vector<Furniture> items; int roomCount=1; int maxItems=20; int expansionCost=500; };
class HousingSystem {
private:
    std::unordered_map<std::string,House> m_Houses; std::vector<Furniture> m_Shop;
    std::function<void(const std::string&,const Furniture&)> m_OnPlace;
public:
    HousingSystem(){ m_Shop={{"chair","Chair","seat",0,0,0,50},{"table","Table","surface",0,0,0,100},{"bed","Bed","sleep",0,0,0,200},{"lamp","Lamp","light",0,0,0,30},{"carpet","Carpet","floor",0,0,0,80},{"painting","Painting","wall",0,0,0,150},{"bookshelf","Bookshelf","storage",0,0,0,120},{"tv","TV","entertainment",0,0,0,300}}; }
    bool BuyFurniture(const std::string& ownerId,int shopIndex){
        if(shopIndex<0||shopIndex>=m_Shop.size())return false;
        auto& item=m_Shop[shopIndex]; auto& house=m_Houses[ownerId];
        if(house.items.size()>=house.maxItems)return false;
        house.items.push_back(item); if(m_OnPlace)m_OnPlace(ownerId,item); return true;
    }
    bool ExpandHouse(const std::string& ownerId){ auto& h=m_Houses[ownerId]; h.roomCount++; h.maxItems+=10; return true; }
    int GetItemCount(const std::string& ownerId)const{ auto it=m_Houses.find(ownerId); return it!=m_Houses.end()?it->second.items.size():0; }
    const auto& GetShop()const{ return m_Shop; }
    void SetOnPlace(std::function<void(const std::string&,const Furniture&)> cb){ m_OnPlace=cb; }
};
}
