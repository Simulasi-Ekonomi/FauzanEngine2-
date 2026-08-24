#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace NeoEngine {
struct Ingredient { std::string name; int count=0; };
struct Recipe { std::string name; std::vector<std::pair<std::string,int>> ingredients; int cookTime=5; int sellValue=10; int xpReward=5; int successRate=80; };
class CookingSystem {
private:
    std::vector<Ingredient> m_Ingredients; std::vector<Recipe> m_Recipes; int m_XP=0,m_Level=1;
    std::function<void(const Recipe&,bool)> m_OnCook;
public:
    CookingSystem(){ m_Recipes={{"Fried Rice",{{"rice",2},{"egg",1},{"oil",1}},5,15,5,85},{"Pizza",{{"flour",3},{"cheese",2},{"tomato",1}},10,30,10,75},{"Steak",{{"meat",2},{"butter",1},{"spices",1}},8,25,8,70},{"Sushi",{{"rice",1},{"fish",2},{"seaweed",1}},4,20,7,90},{"Cake",{{"flour",2},{"egg",2},{"sugar",1},{"milk",1}},12,40,15,65}}; }
    void AddIngredient(const std::string& name,int count){ for(auto& i:m_Ingredients){ if(i.name==name){ i.count+=count;return; } } m_Ingredients.push_back({name,count}); }
    bool HasIngredients(const Recipe& r)const{ for(auto& ing:r.ingredients){ bool found=false; for(auto& i:m_Ingredients){ if(i.name==ing.first&&i.count>=ing.second){ found=true;break; } } if(!found)return false; } return true; }
    bool CookRecipe(int index){ if(index<0||index>=m_Recipes.size())return false; auto& r=m_Recipes[index]; if(!HasIngredients(r))return false;
        for(auto& ing:r.ingredients)for(auto& i:m_Ingredients)if(i.name==ing.first)i.count-=ing.second;
        bool success=(rand()%100)<(r.successRate+m_Level*2);
        if(success){ m_XP+=r.xpReward; if(m_XP>=m_Level*20){ m_XP=0; m_Level++; } }
        if(m_OnCook)m_OnCook(r,success); return success;
    }
    int GetLevel()const{ return m_Level; }
    const auto& GetRecipes()const{ return m_Recipes; }
    void SetOnCook(std::function<void(const Recipe&,bool)> cb){ m_OnCook=cb; }
};
}
