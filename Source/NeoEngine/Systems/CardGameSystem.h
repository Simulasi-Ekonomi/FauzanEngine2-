#pragma once
#include <vector>
#include <string>
#include <cstdlib>
#include <functional>

namespace NeoEngine {
struct Card { std::string id,name; int cost=1; int attack=0,defense=0; std::string effect; int rarity=1; };
struct CardPlayer { std::string id; std::vector<Card> deck,hand,field,graveyard; int hp=30,maxMana=1,currentMana=1; };
class CardGameSystem {
private:
    CardPlayer m_P1,m_P2; CardPlayer* m_CurrentPlayer; bool m_GameOver=false;
    std::function<void(CardPlayer*,CardPlayer*)> m_OnTurn; std::function<void(CardPlayer*)> m_OnWin;
    std::vector<Card> m_CardDB={{"fireball","Fireball",3,5,0,"Deal 5 damage",1},{"shield","Shield",2,0,5,"Gain 5 defense",1},{"dragon","Dragon",8,10,8,"Flying",4},{"healer","Healer",4,3,3,"Heal 2 HP",2},{"knight","Knight",3,3,3,"",1},{"archer","Archer",4,4,2,"Ranged",2}};
public:
    void StartGame(){ m_P1.hp=m_P2.hp=30; m_P1.maxMana=m_P2.maxMana=1; m_P1.currentMana=m_P2.currentMana=1; m_GameOver=false; m_CurrentPlayer=&m_P1; DrawCards(m_P1,3); DrawCards(m_P2,3); }
    void DrawCards(CardPlayer& p,int n=1){ for(int i=0;i<n&&!p.deck.empty();i++){ int idx=rand()%p.deck.size(); p.hand.push_back(p.deck[idx]); p.deck.erase(p.deck.begin()+idx); } }
    void AddCardToDeck(CardPlayer& p,const std::string& name){ for(auto& c:m_CardDB)if(c.name==name){ p.deck.push_back(c);break; } }
    bool PlayCard(CardPlayer& p,int handIndex){
        if(handIndex<0||handIndex>=p.hand.size())return false;
        auto& c=p.hand[handIndex]; if(c.cost>p.currentMana)return false;
        p.currentMana-=c.cost; p.field.push_back(c); p.hand.erase(p.hand.begin()+handIndex); return true;
    }
    void Attack(CardPlayer& attacker,CardPlayer& defender){
        int totalAttack=0; for(auto& c:attacker.field)totalAttack+=c.attack;
        int totalDefense=0; for(auto& c:defender.field)totalDefense+=c.defense;
        int dmg=totalAttack-totalDefense; if(dmg>0)defender.hp-=dmg;
        if(defender.hp<=0){ defender.hp=0; m_GameOver=true; if(m_OnWin)m_OnWin(attacker.hp>0?&attacker:&defender); }
    }
    void EndTurn(){ if(m_CurrentPlayer==&m_P1){ m_CurrentPlayer=&m_P2; m_P2.maxMana++; m_P2.currentMana=m_P2.maxMana; DrawCards(m_P2); } else { m_CurrentPlayer=&m_P1; m_P1.maxMana++; m_P1.currentMana=m_P1.maxMana; DrawCards(m_P1); } }
    CardPlayer* GetPlayer1(){ return &m_P1; } CardPlayer* GetPlayer2(){ return &m_P2; }
    bool IsGameOver()const{ return m_GameOver; }
    void SetOnWin(std::function<void(CardPlayer*)> cb){ m_OnWin=cb; }
};
}
