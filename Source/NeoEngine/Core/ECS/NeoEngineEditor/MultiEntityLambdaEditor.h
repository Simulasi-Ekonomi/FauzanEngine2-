#pragma once
#include "UltimateReactiveLambda.h"
#include <imgui.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

// ============================================================
// Multi-Entity Lambda Editor GUI
class MultiEntityLambdaEditor {
private:
    Registry& registry;
    LiveEditorLambda& editor;

    struct LambdaEntry {
        Entity entity;
        std::string name;
        std::function<void(Entity,Registry&,float)> lambda;
        bool selected = false; // dipilih untuk multi-edit
    };

    [[maybe_unused]] std::vector<LambdaEntry> lambdaEntries;

public:
    MultiEntityLambdaEditor(Registry& reg, LiveEditorLambda& ed)
        : registry(reg), editor(ed) {}

    void AddEntity(Entity e, const std::string& name,
                   std::function<void(Entity,Registry&,float)> lambda) {
        lambdaEntries.push_back({e,name,lambda});
        editor.RunLambda(e, lambda);
    }

    void Render() {
        ImGui::Begin("Multi-Entity Lambda Editor");

        // Pilih semua entity
        if(ImGui::Button("Select All")) {
            for(auto& entry : lambdaEntries) entry.selected = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Deselect All")) {
            for(auto& entry : lambdaEntries) entry.selected = false;
        }

        ImGui::Separator();

        for(auto& entry : lambdaEntries) {
            if(!registry.IsAlive(entry.entity)) continue;

            ImGui::PushID(entry.entity.index);
            ImGui::Checkbox(entry.name.c_str(), &entry.selected);

            // Drag & drop komponen
            if(registry.HasComponent<Position>(entry.entity)) {
                auto& pos = registry.GetComponent<Position>(entry.entity);
                ImGui::DragFloat2("Position",&pos.x,0.1f);
            }

            if(registry.HasComponent<Velocity>(entry.entity)) {
                auto& vel = registry.GetComponent<Velocity>(entry.entity);
                ImGui::DragFloat2("Velocity",&vel.x,0.1f);
            }

            ImGui::PopID();
        }

        ImGui::Separator();

        // Multi-entity lambda assignment
        if(ImGui::Button("Assign Speed Boost Lambda")) {
            auto lambda = [](Entity e, Registry& reg, float dt){
                auto& pos = reg.GetComponent<Position>(e);
                auto& vel = reg.GetComponent<Velocity>(e);
                pos.x += vel.x*dt*2.0f;
                pos.y += vel.y*dt*2.0f;
            };

            for(auto& entry : lambdaEntries) {
                if(entry.selected) {
                    editor.RunLambda(entry.entity, lambda);
                    entry.lambda = lambda;
                }
            }
        }

        ImGui::End();
    }
};
