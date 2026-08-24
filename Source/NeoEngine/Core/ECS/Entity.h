#pragma once
#include <cstdint>
#include <functional>

namespace NeoEngine {
using EntityID = uint32_t;
constexpr EntityID INVALID_ENTITY = 0;

class Entity {
public:
    Entity() : id(INVALID_ENTITY) {}
    explicit Entity(EntityID id) : id(id) {}
    EntityID GetID() const { return id; }
    bool IsValid() const { return id != INVALID_ENTITY; }
    bool operator==(const Entity& other) const { return id == other.id; }
    bool operator!=(const Entity& other) const { return id != other.id; }
private:
    EntityID id;
};
} // namespace NeoEngine

template<>
struct std::hash<NeoEngine::EntityID> {
    size_t operator()(const NeoEngine::EntityID& id) const noexcept {
        return std::hash<uint32_t>{}(id);
    }
};

template<>
struct std::hash<NeoEngine::Entity> {
    size_t operator()(const NeoEngine::Entity& e) const noexcept {
        return std::hash<NeoEngine::EntityID>{}(e.GetID());
    }
};
