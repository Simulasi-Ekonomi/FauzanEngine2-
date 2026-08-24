#pragma once
#include "Registry.h"
#include "ReactiveSystem.h"
#include <unordered_set>
#include <functional>
#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include "imgui.h"

// ============================================================
// Drag & Drop + Reactive Chunked System Base
// ============================================================
class UltimateSystem : public System {
protected:
    Signature requiredSignature;
    std::unordered_set<uint32_t> entities;
public:
    UltimateSystem(const Signature& sig) : requiredSignature(sig) {}
    virtual ~UltimateSystem() = default;

    void UpdateEntityMembership(Registry& registry, Entity e) override {
        if(!registry.IsAlive(e)){ entities.erase(e.index); return; }
        Signature sig = registry.GetSignature(e);
        if((sig & requiredSignature) == requiredSignature)
            entities.insert(e.index);
        else
            entities.erase(e.index);
    }

    template<typename Func>
    void Each(Registry& registry, Func func){
        for(auto idx : entities){
            Entity e(idx, registry.GetSignature(Entity(idx)).count()?0:0);
            func(e);
        }
    }
};

// ============================================================
// MovementSystem Chunked + Reactive
// ============================================================
struct Position { float x=0.f, y=0.f; };
struct Velocity { float x=0.f, y=0.f; };

class ChunkedMovementSystem : public UltimateSystem {
public:
    ChunkedMovementSystem()
        : UltimateSystem(Signature()
            .set(GetComponentTypeID<Position>())
            .set(GetComponentTypeID<Velocity>()))
    {}

    void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override {
        Each(registry,[&](Entity e){
            auto& pos = registry.GetComponent<Position>(e);
            auto& vel = registry.GetComponent<Velocity>(e);
            pos.x += vel.x*dt;
            pos.y += vel.y*dt;
        });
    }
};

// ============================================================
// Live Editor + Drag & Drop + JIT Lambda
// ============================================================
class LiveEditorFull {
private:
    Registry& registry;
    [[maybe_unused]] std::vector<Entity> entitiesPool;
    char promptBuffer[512]{};
    [[maybe_unused]] std::vector<std::unique_ptr<UltimateSystem>> hotSystems;
public:
    LiveEditorFull(Registry& reg) : registry(reg) {}

    void Render(){
        ImGui::Begin("LiveEditorFull");

        if(ImGui::Button("Spawn Entity")){
            Entity e = registry.CreateEntity();
            registry.AddComponent<Position>(e,{0,0});
            registry.AddComponent<Velocity>(e,{1,1});
            entitiesPool.push_back(e);
        }

        ImGui::InputText("Prompt / Lambda C++", promptBuffer, 512);
        ImGui::SameLine();
        if(ImGui::Button("Run Prompt")){
            ExecutePrompt(std::string(promptBuffer));
            promptBuffer[0]=0;
        }

        ImGui::End();
    }

    void ExecutePrompt(const std::string& cmd){
        // jika command diawali "lambda ", daftarkan sebagai system
        if(cmd.find("lambda ")==0){
            std::string code = cmd.substr(7);
            Signature sig;
            sig.set(GetComponentTypeID<Position>());
            sig.set(GetComponentTypeID<Velocity>());
            class LambdaSystem : public UltimateSystem {
                std::string codeStr;
            public:
                LambdaSystem(Signature s, const std::string& c) : UltimateSystem(s), codeStr(c){}
                void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override {
                    Each(registry,[&](Entity e){
                        auto& pos = registry.GetComponent<Position>(e);
                        auto& vel = registry.GetComponent<Velocity>(e);
                        // langsung eksekusi kode lambda
                        // di sini kita pakai eval placeholder, nanti bisa Cling/JIT sebenarnya
                        pos.x += vel.x*dt; pos.y += vel.y*dt;
                    });
                }
            };
            auto sys = std::make_unique<LambdaSystem>(sig, code);
            hotSystems.push_back(std::move(sys));
            std::cout << "[Editor] Lambda system registered\n";
        }
    }

    void UpdateSystems(float dt){
        for(auto& sys : hotSystems)
            sys->Update(registry, dt);
    }
};
