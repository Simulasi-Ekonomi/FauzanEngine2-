#pragma once
#include <string>
#include <cstdlib>
namespace NeoEngine {
struct CombatStats{float health=100,maxHealth=100,attack=10,defense=5,attackSpeed=1,attackRange=2,criticalChance=0.1f,criticalMultiplier=2,lastAttackTime=0;};
struct DamageInfo{float amount;bool critical;std::string element;};
class CombatSystem{
public:
    DamageInfo CalculateDamage(const CombatStats& a,const CombatStats& d){DamageInfo info;float base=a.attack-d.defense*0.5f;if(base<1)base=1;info.critical=(rand()%100)<a.criticalChance*100;info.amount=info.critical?base*a.criticalMultiplier:base;info.element="physical";return info;}
    bool CanAttack(const CombatStats& a,float t){return(t-a.lastAttackTime)>=(1.f/a.attackSpeed);}
    void ApplyDamage(CombatStats& t,const DamageInfo& d){t.health-=d.amount;if(t.health<0)t.health=0;}
    bool IsAlive(const CombatStats& s){return s.health>0;}
    void Heal(CombatStats& s,float a){s.health+=a;if(s.health>s.maxHealth)s.health=s.maxHealth;}
    void LevelUp(CombatStats& s){s.maxHealth+=20;s.health=s.maxHealth;s.attack+=5;s.defense+=3;s.criticalChance+=0.02f;}
};
}
