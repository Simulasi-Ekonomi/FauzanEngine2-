#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <json/json.h>

namespace NeoEngine {

struct SkillDefinition {
    std::string name;
    std::string description;
    std::string category;
    std::vector<std::string> triggers;
    std::function<std::string(const std::string&)> execute;
};

class SkillManager {
public:
    static SkillManager& Get() {
        static SkillManager instance;
        return instance;
    }

    void RegisterSkill(const std::string& name, const std::string& category,
                       const std::string& description,
                       std::function<std::string(const std::string&)> executor) {
        SkillDefinition skill{name, description, category, {}, executor};
        m_Skills[name] = skill;
    }

    std::string ExecuteSkill(const std::string& name, const std::string& input) {
        auto it = m_Skills.find(name);
        if (it != m_Skills.end() && it->second.execute) {
            return it->second.execute(input);
        }
        return "Skill not found: " + name;
    }

    std::vector<std::string> GetSkillsByCategory(const std::string& category) {
        std::vector<std::string> result;
        for (auto& [name, skill] : m_Skills) {
            if (skill.category == category) result.push_back(name);
        }
        return result;
    }

    std::vector<std::string> GetAllSkills() const {
        std::vector<std::string> names;
        for (auto& [name, _] : m_Skills) names.push_back(name);
        return names;
    }

    std::string GetStatusReport() const {
        return "Skills loaded: " + std::to_string(m_Skills.size());
    }

private:
    SkillManager() = default;
    std::unordered_map<std::string, SkillDefinition> m_Skills;
};

} // namespace NeoEngine
