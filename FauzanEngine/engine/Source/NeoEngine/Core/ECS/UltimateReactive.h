#pragma once
#include "Registry.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <string>
#include <memory>

// ============================================================
// Komponen contoh
struct Position { float x=0.0f,y=0.0f; };
struct Velocity { float x=0.0f,y=0.0f; };
struct Physics { float dx=0.0f, dy=0.0f; };
struct Sprite { std::string filename; };
struct AI { int state=0; };

// ============================================================
// Base Reactive Chunked System
class ChunkedReactiveSystem : public System {
protected:
    std::unordered_set<uint32_t> entities; // active entity indices
    Signature requiredSignature;

    using Chunk = std::vector<uint32_t>;
    [[maybe_unused]] std::vector<Chunk> chunks;

public:
    ChunkedReactiveSystem(const Signature& sig) : requiredSignature(sig) {}
    virtual ~ChunkedReactiveSystem() = default;

    void UpdateEntityMembership(Registry& registry, Entity e) override {
        if (!registry.IsAlive(e)) {
            entities.erase(e.index);
            return;
        }
        Signature sig = registry.GetSignature(e);
        if ((sig & requiredSignature) == requiredSignature) {
            entities.insert(e.index);
        } else {
            entities.erase(e.index);
        }
    }

    template<typename Func>
    void Each(Registry& registry, Func func) {
        for (auto idx : entities) {
            func(Entity(idx)).count()?0:0));
        }
    }

    virtual void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override = 0;
};

// ============================================================
// Movement System
class MovementSystem : public ChunkedReactiveSystem {
public:
    MovementSystem() : ChunkedReactiveSystem(
        Signature().set(GetComponentTypeID<Position>())
                   .set(GetComponentTypeID<Velocity>())
    ) {}

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
// Physics System
class PhysicsSystem : public ChunkedReactiveSystem {
public:
    PhysicsSystem() : ChunkedReactiveSystem(
        Signature().set(GetComponentTypeID<Position>())
                   .set(GetComponentTypeID<Physics>())
    ) {}

    void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override {
        Each(registry,[&](Entity e){
            auto& pos = registry.GetComponent<Position>(e);
            auto& phys = registry.GetComponent<Physics>(e);
            pos.x += phys.dx*dt;
            pos.y += phys.dy*dt;
        });
    }
};

// ============================================================
// Render System
class RenderSystem : public ChunkedReactiveSystem {
public:
    RenderSystem() : ChunkedReactiveSystem(
        Signature().set(GetComponentTypeID<Position>())
                   .set(GetComponentTypeID<Sprite>())
    ) {}

    void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override {
        Each(registry,[&](Entity e){
            auto& pos = registry.GetComponent<Position>(e);
            auto& sprite = registry.GetComponent<Sprite>(e);
            // rendering placeholder
            // Renderer::Draw(sprite.filename,pos.x,pos.y);
        });
    }
};

// ============================================================
// AI System
class AISystem : public ChunkedReactiveSystem {
public:
    AISystem() : ChunkedReactiveSystem(
        Signature().set(GetComponentTypeID<Position>())
                   .set(GetComponentTypeID<AI>())
    ) {}

    void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override {
        Each(registry,[&](Entity e){
            auto& ai = registry.GetComponent<AI>(e);
            ai.state++; // placeholder logic
        });
    }
};

// ============================================================
// Live Editor Ultimate 4
class LiveEditorUltimate4 {
private:
    Registry& registry;
    [[maybe_unused]] std::vector<std::unique_ptr<ChunkedReactiveSystem>> systems;
public:
    LiveEditorUltimate4(Registry& reg) : registry(reg) {}

    template<typename T, typename... Args>
    T* RegisterSystem(Args&&... args) {
        auto sys = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = sys.get();
        systems.push_back(std::move(sys));
        return ptr;
    }

    void Render() {
        // Render editor UI + drag/drop entity
    }

    void UpdateSystems(float dt) {
        for (auto& sys : systems) sys->Update(registry, dt);
    }
};
