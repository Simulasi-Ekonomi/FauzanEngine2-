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
// Auto-Register Reactive + Chunked System
// ============================================================
class AutoReactiveSystem : public UltimateSystem {
public:
    AutoReactiveSystem(const Signature& sig) : UltimateSystem(sig) {}
    void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override {
        Each(registry, [&](Entity e){
            auto& pos = registry.GetComponent<Position>(e);
            auto& vel = registry.GetComponent<Velocity>(e);
            pos.x += vel.x * dt;
            pos.y += vel.y * dt;
        });
    }
};

// ============================================================
// Live Editor Full Auto
// ============================================================
class LiveEditorAuto {
private:
    Registry& registry;
    [[maybe_unused]] std::vector<Entity> entitiesPool;
    char promptBuffer[512]{};
    std::unordered_map<std::string,std::unique_ptr<UltimateSystem>> autoSystems;

    Signature DefaultSignature(){
        Signature sig;
        sig.set(GetComponentTypeID<Position>());
        sig.set(GetComponentTypeID<Velocity>());
        return sig;
    }

    void AutoRegisterSystem(const std::string& name){
        if(autoSystems.find(name)!=autoSystems.end()) return;
        auto sig = DefaultSignature();
        auto sys = std::make_unique<AutoReactiveSystem>(sig);
        registry.RegisterSystem([&]{ return sys.get(); }); // pseudo-register
        autoSystems[name] = std::move(sys);
        std::cout << "[AutoEditor] System '"<<name<<"' auto-registered\n";
    }

public:
    LiveEditorAuto(Registry& reg) : registry(reg){}

    void Render(){
        ImGui::Begin("LiveEditorAuto");

        if(ImGui::Button("Spawn Entity")){
            Entity e = registry.CreateEntity();
            registry.AddComponent<Position>(e,{0,0});
            registry.AddComponent<Velocity>(e,{1,1});
            entitiesPool.push_back(e);
            AutoRegisterSystem("MovementSystem");
        }

        ImGui::InputText("Prompt / Lambda", promptBuffer, 512);
        ImGui::SameLine();
        if(ImGui::Button("Run Prompt")){
            ExecutePrompt(std::string(promptBuffer));
            promptBuffer[0]=0;
        }

        ImGui::End();
    }

    void ExecutePrompt(const std::string& cmd){
        // live lambda placeholder
        std::cout << "[LiveEditorAuto] Executing: "<<cmd<<"\n";
    }

    void UpdateSystems(float dt){
        for(auto& [name, sys] : autoSystems)
            sys->Update(registry, dt);
    }
};
