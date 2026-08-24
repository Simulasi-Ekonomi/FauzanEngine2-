#pragma once
#include "Registry.h"
#include "ReactiveSystem.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <iostream>
#include "imgui.h"

// ============================================================
// Ultimate 4.0 Live Editor + Auto-Chunks + Live Lambda ECS
// ============================================================
class LiveEditorUltimate4 {
private:
    Registry& registry;
    [[maybe_unused]] std::vector<Entity> entitiesPool;
    char promptBuffer[1024]{};

    struct SystemEntry {
        std::unique_ptr<ReactiveSystem> system;
        Signature signature;
        [[maybe_unused]] std::vector<uint32_t> chunkIndices; // entity indices grouped by chunk
    };
    std::unordered_map<std::string,SystemEntry> systemsPool;

    void AutoRegisterCoreSystems(){
        if(systemsPool.empty()){
            auto* moveSys = registry.RegisterSystem<ChunkedMovementSystem>();
            systemsPool["Movement"] = {std::unique_ptr<ReactiveSystem>(moveSys), moveSys->GetRequiredSignature(), {}};

            auto* physicsSys = registry.RegisterSystem<ChunkedPhysicsSystem>();
            systemsPool["Physics"] = {std::unique_ptr<ReactiveSystem>(physicsSys), physicsSys->GetRequiredSignature(), {}};

            auto* renderSys = registry.RegisterSystem<ChunkedRenderSystem>();
            systemsPool["Render"] = {std::unique_ptr<ReactiveSystem>(renderSys), renderSys->GetRequiredSignature(), {}};

            auto* aiSys = registry.RegisterSystem<ChunkedAISystem>();
            systemsPool["AI"] = {std::unique_ptr<ReactiveSystem>(aiSys), aiSys->GetRequiredSignature(), {}};

            std::cout<<"[LiveEditorUltimate4] Core chunked systems auto-registered\n";
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
        AutoUpdateEntityMembership(e);
    }

    void AutoUpdateEntityMembership(Entity e){
        for(auto& [name, entry] : systemsPool){
            entry.system->UpdateEntityMembership(registry,e);
            // Update chunk indices
            entry.chunkIndices.clear();
            for(uint32_t idx : entry.system->GetEntities()){
                entry.chunkIndices.push_back(idx);
            }
        }
    }

    void ExecuteLambda(const std::string& code){
        // Placeholder: user lambda runtime execution
        // bisa dieksekusi via scripting engine (C++ eval / Lua / Wasm)
        std::cout << "[LiveEditorUltimate4] Executing Lambda: "<<code<<"\n";
    }

public:
    LiveEditorUltimate4(Registry& reg) : registry(reg){
        AutoRegisterCoreSystems();
    }

    void Render(){
        ImGui::Begin("LiveEditorUltimate4");

        if(ImGui::Button("Spawn Default Entity")){
            SpawnDefaultEntity();
        }

        ImGui::InputTextMultiline("Lambda / Prompt", promptBuffer, 1024);
        if(ImGui::Button("Run Lambda")){
            ExecuteLambda(std::string(promptBuffer));
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

        if(ImGui::CollapsingHeader("Systems Chunks")){
            for(auto& [name, entry] : systemsPool){
                ImGui::Text("%s system chunks: %zu", name.c_str(), entry.chunkIndices.size());
            }
        }

        ImGui::End();
    }

    void UpdateSystems(float dt){
        for(auto& [name, entry] : systemsPool){
            entry.system->Update(registry, dt);
        }
    }

    const std::vector<Entity>& GetEntities() const { return entitiesPool; }
};
