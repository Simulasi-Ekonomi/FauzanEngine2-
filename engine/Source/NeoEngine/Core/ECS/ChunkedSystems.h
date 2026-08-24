#pragma once
#include "ReactiveSystem.h"
#include "Registry.h"
#include <vector>
#include <string>
#include <unordered_map>

// ============================================================
// Contoh komponen
struct Position { float x=0.0f, y=0.0f; };
struct Velocity { float x=0.0f, y=0.0f; };
struct Physics { float dx=0.0f, dy=0.0f; };
struct Sprite { std::string filename; };
struct AI { int state=0; };

// ============================================================
// Movement System
class ChunkedMovementSystem : public ReactiveSystem {
private:
    using Chunk = std::vector<uint32_t>; // entity indices
    [[maybe_unused]] std::vector<Chunk> chunks;
public:
    ChunkedMovementSystem() : ReactiveSystem(Signature()
        .set(GetComponentTypeID<Position>())
        .set(GetComponentTypeID<Velocity>())) {}

    void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override {
        Each(registry, [&](Entity e){
            auto& pos = registry.GetComponent<Position>(e);
            auto& vel = registry.GetComponent<Velocity>(e);
            pos.x += vel.x * dt;
            pos.y += vel.y * dt;
        });
    }

    const Signature& GetRequiredSignature() const { return requiredSignature; }
    const std::vector<uint32_t>& GetEntities() const {
        static std::vector<uint32_t> flat;
        flat.clear();
        for(auto& c: chunks) flat.insert(flat.end(), c.begin(), c.end());
        return flat;
    }
};

// ============================================================
// Physics System
class ChunkedPhysicsSystem : public ReactiveSystem {
public:
    ChunkedPhysicsSystem() : ReactiveSystem(Signature()
        .set(GetComponentTypeID<Position>())
        .set(GetComponentTypeID<Physics>())) {}

    void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override {
        Each(registry, [&](Entity e){
            auto& pos = registry.GetComponent<Position>(e);
            auto& phys = registry.GetComponent<Physics>(e);
            pos.x += phys.dx * dt;
            pos.y += phys.dy * dt;
        });
    }

    const Signature& GetRequiredSignature() const { return requiredSignature; }
};

// ============================================================
// Render System
class ChunkedRenderSystem : public ReactiveSystem {
public:
    ChunkedRenderSystem() : ReactiveSystem(Signature()
        .set(GetComponentTypeID<Position>())
        .set(GetComponentTypeID<Sprite>())) {}

    void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override {
        Each(registry, [&](Entity e){
            auto& pos = registry.GetComponent<Position>(e);
            auto& sprite = registry.GetComponent<Sprite>(e);
            // render placeholder
            // e.g. Renderer::Draw(sprite.filename, pos.x, pos.y);
        });
    }

    const Signature& GetRequiredSignature() const { return requiredSignature; }
};

// ============================================================
// AI System
class ChunkedAISystem : public ReactiveSystem {
public:
    ChunkedAISystem() : ReactiveSystem(Signature()
        .set(GetComponentTypeID<Position>())
        .set(GetComponentTypeID<AI>())) {}

    void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override {
        Each(registry, [&](Entity e){
            auto& ai = registry.GetComponent<AI>(e);
            ai.state++; // placeholder AI logic
        });
    }

    const Signature& GetRequiredSignature() const { return requiredSignature; }
};
