#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace NeoEngine {

struct CraftingRecipe {
    std::string name;
    std::string resultItem;
    int resultCount = 1;
    std::unordered_map<std::string, int> ingredients;
    float craftingTime = 2.0f;
};

class CraftingSystem {
public:
    void AddRecipe(const CraftingRecipe& recipe);
    const CraftingRecipe* GetRecipe(const std::string& name) const;
    bool CanCraft(const CraftingRecipe& recipe, const std::unordered_map<std::string, int>& inventory) const;
    std::vector<CraftingRecipe> GetAllRecipes() const;
private:
    std::vector<CraftingRecipe> m_Recipes;
};

} // namespace NeoEngine
