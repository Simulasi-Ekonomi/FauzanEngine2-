#include "CraftingSystem.h"

namespace NeoEngine {

void CraftingSystem::AddRecipe(const CraftingRecipe& recipe) { m_Recipes.push_back(recipe); }

const CraftingRecipe* CraftingSystem::GetRecipe(const std::string& name) const {
    for (auto& r : m_Recipes) if (r.name == name) return &r;
    return nullptr;
}

bool CraftingSystem::CanCraft(const CraftingRecipe& recipe, const std::unordered_map<std::string, int>& inventory) const {
    for (auto& [item, count] : recipe.ingredients) {
        auto it = inventory.find(item);
        if (it == inventory.end() || it->second < count) return false;
    }
    return true;
}

std::vector<CraftingRecipe> CraftingSystem::GetAllRecipes() const { return m_Recipes; }

} // namespace NeoEngine
