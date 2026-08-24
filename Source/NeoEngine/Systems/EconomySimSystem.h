#pragma once
#include <vector>
#include <string>
#include <unordered_map>

namespace NeoEngine {
struct Stock { std::string id,name; float price=100; float volatility=0.02f; int owned=0; float history[10]={100}; };
class EconomySimSystem {
private:
    std::vector<Stock> m_Stocks; float m_Cash=10000; float m_PortfolioValue=0; int m_Day=0;
public:
    EconomySimSystem(){ m_Stocks={{"tech","Tech Corp",150,0.03f},{"food","Food Chain",80,0.01f},{"energy","Energy Ltd",200,0.05f},{"bank","Bank Inc",120,0.02f},{"game","Game Studio",50,0.08f}}; }
    void SimulateDay(){ m_Day++;
        for(auto& s:m_Stocks){ float change=(rand()%200-100)/10000.0f*s.volatility; s.price*=(1+change); if(s.price<1)s.price=1; for(int i=9;i>0;i--)s.history[i]=s.history[i-1]; s.history[0]=s.price; }
        m_PortfolioValue=m_Cash; for(auto& s:m_Stocks)m_PortfolioValue+=s.price*s.owned;
    }
    bool BuyStock(int index,int amount){ if(index<0||index>=m_Stocks.size())return false; auto& s=m_Stocks[index]; float cost=s.price*amount; if(m_Cash<cost)return false; m_Cash-=cost; s.owned+=amount; return true; }
    bool SellStock(int index,int amount){ if(index<0||index>=m_Stocks.size())return false; auto& s=m_Stocks[index]; if(s.owned<amount)return false; m_Cash+=s.price*amount; s.owned-=amount; return true; }
    float GetCash()const{ return m_Cash; } float GetPortfolioValue()const{ return m_PortfolioValue; }
    int GetDay()const{ return m_Day; } const auto& GetStocks()const{ return m_Stocks; }
};
}
