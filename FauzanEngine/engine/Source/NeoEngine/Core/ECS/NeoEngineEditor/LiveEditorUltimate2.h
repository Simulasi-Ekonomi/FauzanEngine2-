#pragma once
#include "Registry.h"
#include "ReactiveSystem.h"
#include "MovementSystem.h"
#include "PhysicsSystem.h"
#include "RenderSystem.h"
#include "AISystem.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include "imgui.h"

// ============================================================
// Ultimate 2.0 Live Editor + Auto-Register Systems
// ============================================================
class LiveEditorUltimate2 {
private:
    Registry& registry;
    [[maybe_unused]] std::vector<Entity> entitiesPool;
    char promptBuffer[512]{};
    std::unordered_map<std::string,std::unique_ptr<ReactiveSystem>> systemsPool;

    void AutoRegisterSystems(){
        if(systemsPool.empty()){
            systemsPool["Movement"] = registry.RegisterSystem<MovementSystem>();
            systemsPool["Physics"]  = registry.RegisterSystem<PhysicsSystem>();
            systemsPool["Render"]   = registry.RegisterSystem<RenderSystem>();
            systemsPool["AI"]       = registry.RegisterSystem<AISystem>();
            std::cout<<"[LiveEditorUltimate2] All core systems auto-registered\n";
        }
    }

    void SpawnDefaultEntity(){
        Entity e = registry.CreateEntity();
        registry.AddComponent<Position>(e,{0,0});
        registry.AddComponent<Velocity>(e,{1,1});
        registry.AddComponent<Physics>(e,{0,0});
        registry.AddComponent<Sprite>(e,{"default.png"});
        registry.AddComponent<AI>(e,{});
        entitiesPool.push_back(e);
    }

public:
    LiveEditorUltimate2(Registry& reg) : registry(reg){
        AutoRegisterSystems();
    }

    void Render(){
        ImGui::Begin("LiveEditorUltimate2");

        if(ImGui::Button("Spawn Default Entity")){
            SpawnDefaultEntity();
        }

        ImGui::InputText("Prompt / Lambda", promptBuffer, 512);
        ImGui::SameLine();
        if(ImGui::Button("Run Prompt")){
            ExecutePrompt(std::string(promptBuffer));
            promptBuffer[0]=0;
        }

        // Optional: drag & drop panel for entity list
        if(ImGui::CollapsingHeader("Entities Pool")){
            for(size_t i=0;i<entitiesPool.size();++i){
                Entity e = entitiesPool[i];
                ImGui::Text("Entity %zu: %u", i, e.index);
            }
        }

        ImGui::End();
    }

    void ExecutePrompt(const std::string& cmd){
        // placeholder: live lambda execution
        std::cout << "[LiveEditorUltimate2] Executing: "<<cmd<<"\n";
    }

    void UpdateSystems(float dt){
        for(auto& [name, sys] : systemsPool)
            sys->Update(registry, dt);
    }

    // Helper: get entities pool
    const std::vector<Entity>& GetEntities() const { return entitiesPool; }
};
