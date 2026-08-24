#pragma once
#include "Registry.h"
#include "ReactiveSystem.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include "imgui.h"

// ============================================================
// Ultimate 3.0 Live Editor + Full Auto-Register / Chunked ECS
// ============================================================
class LiveEditorUltimate3 {
private:
    Registry& registry;
    [[maybe_unused]] std::vector<Entity> entitiesPool;
    char promptBuffer[512]{};

    struct SystemEntry {
        std::unique_ptr<ReactiveSystem> system;
        Signature signature;
    };
    std::unordered_map<std::string,SystemEntry> systemsPool;

    void AutoRegisterCoreSystems(){
        // Core systems: Movement, Physics, Render, AI
        if(systemsPool.empty()){
            auto* moveSys = registry.RegisterSystem<MovementSystem>();
            systemsPool["Movement"] = {std::unique_ptr<ReactiveSystem>(moveSys), moveSys->GetRequiredSignature()};

            auto* physicsSys = registry.RegisterSystem<PhysicsSystem>();
            systemsPool["Physics"] = {std::unique_ptr<ReactiveSystem>(physicsSys), physicsSys->GetRequiredSignature()};

            auto* renderSys = registry.RegisterSystem<RenderSystem>();
            systemsPool["Render"] = {std::unique_ptr<ReactiveSystem>(renderSys), renderSys->GetRequiredSignature()};

            auto* aiSys = registry.RegisterSystem<AISystem>();
            systemsPool["AI"] = {std::unique_ptr<ReactiveSystem>(aiSys), aiSys->GetRequiredSignature()};

            std::cout<<"[LiveEditorUltimate3] Core systems auto-registered\n";
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

    void AutoUpdateEntityMembership(Entity e){
        // Iterasi semua reactive system → update entity membership
        for(auto& [name, entry] : systemsPool){
            entry.system->UpdateEntityMembership(registry,e);
        }
    }

public:
    LiveEditorUltimate3(Registry& reg) : registry(reg){
        AutoRegisterCoreSystems();
    }

    void Render(){
        ImGui::Begin("LiveEditorUltimate3");

        if(ImGui::Button("Spawn Default Entity")){
            SpawnDefaultEntity();
            AutoUpdateEntityMembership(entitiesPool.back());
        }

        ImGui::InputText("Prompt / Lambda", promptBuffer, 512);
        ImGui::SameLine();
        if(ImGui::Button("Run Prompt")){
            ExecutePrompt(std::string(promptBuffer));
            promptBuffer[0]=0;
        }

        if(ImGui::CollapsingHeader("Entities Pool")){
            for(size_t i=0;i<entitiesPool.size();++i){
                Entity e = entitiesPool[i];
                ImGui::Text("Entity %zu: %u", i, e.index);
                if(ImGui::Button(("Add Velocity##"+std::to_string(i)).c_str())){
                    registry.AddComponent<Velocity>(e,{0,0});
                    AutoUpdateEntityMembership(e);
                }
                if(ImGui::Button(("Remove Velocity##"+std::to_string(i)).c_str())){
                    registry.RemoveComponent<Velocity>(e);
                    AutoUpdateEntityMembership(e);
                }
            }
        }

        ImGui::End();
    }

    void ExecutePrompt(const std::string& cmd){
        // Placeholder: user prompt runtime
        std::cout << "[LiveEditorUltimate3] Executing: "<<cmd<<"\n";
        // contoh: user bisa menambahkan komponen via prompt
    }

    void UpdateSystems(float dt){
        for(auto& [name, entry] : systemsPool){
            entry.system->Update(registry, dt);
        }
    }

    const std::vector<Entity>& GetEntities() const { return entitiesPool; }
};
