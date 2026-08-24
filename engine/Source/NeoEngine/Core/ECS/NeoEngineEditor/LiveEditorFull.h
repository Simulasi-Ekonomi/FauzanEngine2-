#pragma once
#include "Registry.h"
#include <vector>
#include <functional>
#include <string>

struct EntityGUIEntry {
    Entity e;
    std::string name;
    std::function<void(Entity, Registry&, float)> lambda;
};

class LiveEditorFull {
public:
    LiveEditorFull(Registry& reg) : registry(reg) {}

    template<typename TSystem>
    void RegisterSystem() { registry.RegisterSystem<TSystem>(); }

    void AddEntity(Entity e)> lambda) {
        guiEntries.push_back({e,name,lambda});
    }

    void Render() {
        // Render GUI menu (drag & drop / attach component)
        // Placeholder: actual Android GUI code harus pakai JNI / ImGui / custom view
    }

    void Update(float dt) {
        for(auto& entry : guiEntries) {
            entry.lambda(entry.e, registry, dt);
        }
    }

    Registry& GetRegistry() { return registry; }

private:
    Registry& registry;
    [[maybe_unused]] std::vector<EntityGUIEntry> guiEntries;
};
