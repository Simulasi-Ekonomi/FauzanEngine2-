#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <json/json.h>

namespace NeoEngine {

struct GameDesignPrompt {
    std::string category;
    std::string name;
    std::string defaultValue;
    std::string description;
    std::vector<std::string> options; // untuk dropdown
    std::string type; // "text", "number", "dropdown", "slider", "file", "textarea"
    float min, max; // untuk slider
};

struct GameDesignDocument {
    std::string gameTitle;
    std::string genre;
    std::string description;
    int targetFPS = 60;
    int worldSizeKm = 10;
    int maxNPCs = 100;
    int maxQuests = 50;
    std::string monetization;
    std::vector<std::string> features;
    std::unordered_map<std::string, std::string> customParams;
};

class EditorPromptSystem {
private:
    std::vector<GameDesignPrompt> m_Prompts;
    GameDesignDocument m_CurrentDesign;
    std::function<void(const GameDesignDocument&)> m_OnGenerate;
    std::function<void(const std::string&)> m_OnImport;
    
public:
    EditorPromptSystem() {
        SetupDefaultPrompts();
    }
    
    void SetupDefaultPrompts() {
        m_Prompts = {
            {"Game Info", "gameTitle", "My Awesome Game", "Nama game Anda", {}, "text", 0, 0},
            {"Game Info", "genre", "RPG", "Genre utama", {"RPG","FPS","Strategy","Simulation","Tower Defense","Battle Royale","Idle","Survival","MMORPG"}, "dropdown", 0, 0},
            {"Game Info", "description", "A game about...", "Deskripsi game (maks 500 karakter)", {}, "textarea", 0, 0},
            {"Technical", "targetFPS", "60", "Target FPS", {"30","60","120"}, "dropdown", 0, 0},
            {"Technical", "worldSizeKm", "10", "Ukuran dunia (km)", {}, "slider", 1, 100},
            {"Content", "maxNPCs", "100", "Jumlah maksimal NPC", {}, "slider", 10, 10000},
            {"Content", "maxQuests", "50", "Jumlah maksimal Quest", {}, "slider", 5, 500},
            {"Content", "features", "[]", "Fitur unggulan (pisahkan dengan koma)", {}, "text", 0, 0},
            {"Monetization", "monetization", "Free to Play + IAP", "Model monetisasi", {"Free to Play","Premium","Freemium","VIP Pass"}, "dropdown", 0, 0},
            {"Import", "importFile", "", "Import dokumen Word/PDF game design", {}, "file", 0, 0},
        };
    }
    
    const std::vector<GameDesignPrompt>& GetPrompts() const { return m_Prompts; }
    
    void SetParam(const std::string& name, const std::string& value) {
        if (name == "gameTitle") m_CurrentDesign.gameTitle = value;
        else if (name == "genre") m_CurrentDesign.genre = value;
        else if (name == "description") m_CurrentDesign.description = value;
        else if (name == "targetFPS") m_CurrentDesign.targetFPS = std::stoi(value);
        else if (name == "worldSizeKm") m_CurrentDesign.worldSizeKm = std::stoi(value);
        else if (name == "maxNPCs") m_CurrentDesign.maxNPCs = std::stoi(value);
        else if (name == "maxQuests") m_CurrentDesign.maxQuests = std::stoi(value);
        else if (name == "monetization") m_CurrentDesign.monetization = value;
        else if (name == "features") {
            m_CurrentDesign.features.clear();
            std::string f = value;
            size_t pos = 0;
            while ((pos = f.find(",")) != std::string::npos) {
                m_CurrentDesign.features.push_back(f.substr(0, pos));
                f.erase(0, pos + 1);
            }
            if (!f.empty()) m_CurrentDesign.features.push_back(f);
        }
        else m_CurrentDesign.customParams[name] = value;
    }
    
    void ImportDocument(const std::string& path) {
        if (m_OnImport) m_OnImport(path);
    }
    
    void GenerateGame() {
        if (m_OnGenerate) m_OnGenerate(m_CurrentDesign);
    }
    
    GameDesignDocument& GetCurrentDesign() { return m_CurrentDesign; }
    void SetOnGenerate(std::function<void(const GameDesignDocument&)> cb) { m_OnGenerate = cb; }
    void SetOnImport(std::function<void(const std::string&)> cb) { m_OnImport = cb; }
};

} // namespace NeoEngine
