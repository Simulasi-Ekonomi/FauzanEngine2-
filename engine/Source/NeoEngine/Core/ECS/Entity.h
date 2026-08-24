#pragma once
#include <cstdint>

namespace NeoEngine {

struct Entity
{
    uint32_t id;

    Entity(uint32_t i = 0) : id(i) {}

    bool operator==(const Entity& other) const noexcept
    {
        return id == other.id;
    }

    bool operator!=(const Entity& other) const noexcept
    {
        return id != other.id;
    }
};

}

namespace std {
template<>
struct hash<NeoEngine::Entity>
{
    size_t operator()(const NeoEngine::Entity& e) const noexcept
    {
        return std::hash<uint32_t>()(e.id);
    }
};
}
