#pragma once

#include "Registry.h"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <bitset>

// Base reactive system cache-friendly: archetype-chunk
class ChunkedReactiveSystem : public System {
protected:
    struct Chunk {
        [[maybe_unused]] std::vector<uint32_t> entityIndices;  // index entity dalam registry
    };

    [[maybe_unused]] std::vector<Chunk> chunks;               // kumpulan chunk aktif
    Signature requiredSignature;             // signature filter

public:
    ChunkedReactiveSystem(const Signature& sig)
        : requiredSignature(sig)
    {}

    virtual ~ChunkedReactiveSystem() = default;

    // Dipanggil Registry saat entity update
    virtual void UpdateEntityMembership(Registry& registry, Entity e) override {
        if (!registry.IsAlive(e)) {
            // hapus dari semua chunk
            for (auto& chunk : chunks) {
                auto it = std::remove(chunk.entityIndices.begin(),
                                      chunk.entityIndices.end(), e.index);
                chunk.entityIndices.erase(it, chunk.entityIndices.end());
            }
            return;
        }

        const Signature sig = registry.GetSignature(e);
        bool matches = (sig & requiredSignature) == requiredSignature;

        bool found = false;
        for (auto& chunk : chunks) {
            auto it = std::find(chunk.entityIndices.begin(), chunk.entityIndices.end(), e.index);
            if (it != chunk.entityIndices.end()) {
                found = true;
                if (!matches) {
                    // leave system
                    chunk.entityIndices.erase(it);
                }
                break;
            }
        }

        if (!found && matches) {
            // join system: pilih chunk terakhir atau buat baru
            if (chunks.empty() || chunks.back().entityIndices.size() >= 256) {
                chunks.emplace_back();
            }
            chunks.back().entityIndices.push_back(e.index);
        }
    }

    // Iterasi semua entity di sistem (internal)
    template<typename Func>
    void ForEach(Registry& registry, Func func) {
        for (auto& chunk : chunks) {
            for (uint32_t idx : chunk.entityIndices) {
                Entity e(idx, registry.GetSignature(Entity(idx)).count() ? 0 : 0);
                func(e);
            }
        }
    }

    virtual void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override = 0;
};

// Contoh sistem turunan: movement
struct Position { float x=0.0f; float y=0.0f; };
struct Velocity { float x=0.0f; float y=0.0f; };

class ChunkedMovementSystem : public ChunkedReactiveSystem {
public:
    ChunkedMovementSystem()
        : ChunkedReactiveSystem(Signature()
            .set(GetComponentTypeID<Position>())
            .set(GetComponentTypeID<Velocity>()))
    {}

    void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override {
        ForEach(registry, [&](Entity e){
            auto& pos = registry.GetComponent<Position>(e);
            auto& vel = registry.GetComponent<Velocity>(e);
            pos.x += vel.x * dt;
            pos.y += vel.y * dt;
        });
    }
};
