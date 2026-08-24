#pragma once
#include "UltimateReactiveLambda.h"
#include <imgui.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

// ============================================================
// Live Lambda Editor GUI
class LiveLambdaEditorGUI {
private:
    Registry& registry;
    LiveEditorLambda& editor;

    struct LambdaEntry {
        Entity entity;
        std::string name;
        std::function<void(Entity,Registry&,float)> lambda;
    };

    [[maybe_unused]] std::vector<LambdaEntry> lambdaEntries;

public:
    LiveLambdaEditorGUI(Registry& reg, LiveEditorLambda& ed)
        : registry(reg), editor(ed) {}

    // Tambah entity + lambda ke editor GUI
    void AddLambdaEntity(Entity e, const std::string& name,
                         std::function<void(Entity,Registry&,float)> lambda) {
        lambdaEntries.push_back({e,name,lambda});
        editor.RunLambda(e, lambda);
    }

    // Render GUI ImGui
    void Render() {
        ImGui::Begin("Live Lambda Editor");

        for(auto& entry : lambdaEntries) {
            if(!registry.IsAlive(entry.entity))
                continue;

            ImGui::PushID(entry.entity.index);
            ImGui::Text("Entity %s [%d]", entry.name.c_str(), entry.entity.index);

            // Drag & drop Position
            if(registry.HasComponent<Position>(entry.entity)) {
                auto& pos = registry.GetComponent<Position>(entry.entity);
                ImGui::DragFloat2("Position",&pos.x,0.1f);
            }

            // Drag & drop Velocity
            if(registry.HasComponent<Velocity>(entry.entity)) {
                auto& vel = registry.GetComponent<Velocity>(entry.entity);
                ImGui::DragFloat2("Velocity",&vel.x,0.1f);
            }

            // Button untuk ganti lambda behavior
            if(ImGui::Button("Assign Speed Boost Lambda")) {
                auto lambda = [](Entity e, Registry& reg, float dt){
                    auto& pos = reg.GetComponent<Position>(e);
                    auto& vel = reg.GetComponent<Velocity>(e);
                    pos.x += vel.x*dt*2.0f; // speed boost
                    pos.y += vel.y*dt*2.0f;
                };
                editor.RunLambda(entry.entity, lambda);
                entry.lambda = lambda;
            }

            ImGui::Separator();
            ImGui::PopID();
        }

        ImGui::End();
    }
};
