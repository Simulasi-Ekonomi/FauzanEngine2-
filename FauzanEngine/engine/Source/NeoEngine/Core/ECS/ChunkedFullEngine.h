#pragma once
#include "Registry.h"
#include "Entity.h"
#include "System.h"
#include "Signature.h"
#include "ComponentType.h"
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <bitset>
#include <cassert>
#include <memory>
#include <iostream>

using namespace NeoEngine;
// ==========================
// Archetype + Chunked Reactive System
// ==========================
struct ArchetypeChunk {
    [[maybe_unused]] std::vector<uint32_t> entities;
    std::unordered_map<size_t, std::vector<uint8_t>> componentData; // raw bytes
};

class ChunkedReactiveSystem : public System {
protected:
    [[maybe_unused]] std::vector<ArchetypeChunk*> chunks;
    Signature requiredSignature;

public:
    ChunkedReactiveSystem(const Signature& sig) : requiredSignature(sig) {}
    virtual ~ChunkedReactiveSystem() = default;

    void UpdateEntityMembership(Registry& registry, Entity e) override {
        if (!registry.IsAlive(e)) {
            for (auto* chunk : chunks) {
                auto it = std::find(chunk->entities.begin(), chunk->entities.end(), e.index);
                if (it != chunk->entities.end()) {
                    chunk->entities.erase(it);
                }
            }
            return;
        }

        Signature sig = registry.GetSignature(e);
        bool match = (sig & requiredSignature) == requiredSignature;

        for (auto* chunk : chunks) {
            auto it = std::find(chunk->entities.begin(), chunk->entities.end(), e.index);
            if (it != chunk->entities.end() && !match) {
                chunk->entities.erase(it);
                return;
            }
        }

        if (match) {
            // Masukkan ke chunk pertama yang punya ruang, atau buat chunk baru
            if (chunks.empty() || chunks.back()->entities.size() >= 512) {
                chunks.push_back(new ArchetypeChunk());
            }
            chunks.back()->entities.push_back(e.index);
        }
    }

    template<typename Func>
    void Each(Registry& registry, Func func) {
        for (auto* chunk : chunks) {
            for (uint32_t idx : chunk->entities) {
                func(Entity(idx)).count() ? 0 : 0));
            }
        }
    }

    virtual void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override = 0;
};

// ==========================
// Contoh sistem turunan super fast
// ==========================
struct Position { float x=0.0f, y=0.0f; };
struct Velocity { float x=0.0f, y=0.0f; };
struct Acceleration { float ax=0.0f, ay=0.0f; };
struct Health { float value=100.0f; };
struct Renderable { char glyph='@'; };

class ChunkedMovementSystem : public ChunkedReactiveSystem {
public:
    ChunkedMovementSystem()
        : ChunkedReactiveSystem(Signature()
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
};

class ChunkedPhysicsSystem : public ChunkedReactiveSystem {
public:
    ChunkedPhysicsSystem()
        : ChunkedReactiveSystem(Signature()
            .set(GetComponentTypeID<Velocity>())
            .set(GetComponentTypeID<Acceleration>())) {}

    void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override {
        Each(registry, [&](Entity e){
            auto& vel = registry.GetComponent<Velocity>(e);
            auto& acc = registry.GetComponent<Acceleration>(e);
            vel.x += acc.ax * dt;
            vel.y += acc.ay * dt;
        });
    }
};

class ChunkedAISystem : public ChunkedReactiveSystem {
public:
    ChunkedAISystem()
        : ChunkedReactiveSystem(Signature()
            .set(GetComponentTypeID<Position>())
            .set(GetComponentTypeID<Velocity>())
            .set(GetComponentTypeID<Health>())) {}

    void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override {
        Each(registry, [&](Entity e){
            auto& pos = registry.GetComponent<Position>(e);
            auto& vel = registry.GetComponent<Velocity>(e);
            auto& health = registry.GetComponent<Health>(e);

            if (pos.x < 0 || pos.x > 100) vel.x = -vel.x;
            if (pos.y < 0 || pos.y > 100) vel.y = -vel.y;

            health.value -= 0.05f*dt;
        });
    }
};

class ChunkedRenderSystem : public ChunkedReactiveSystem {
public:
    ChunkedRenderSystem()
        : ChunkedReactiveSystem(Signature()
            .set(GetComponentTypeID<Position>())
            .set(GetComponentTypeID<Renderable>())) {}

    void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override {
        Each(registry, [&](Entity e){
            auto& pos = registry.GetComponent<Position>(e);
            auto& render = registry.GetComponent<Renderable>(e);
            if (int(pos.x)%10==0 && int(pos.y)%10==0)
                std::cout<<"Entity at ("<<pos.x<<","<<pos.y<<") glyph="<<render.glyph<<"\n";
        });
    }
};
