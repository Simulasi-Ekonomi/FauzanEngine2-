// EOF
#ifndef NEO_ACTOR_COMPONENT_WORLD_HPP
#define NEO_ACTOR_COMPONENT_WORLD_HPP

#include <cstdint>
#include <cstddef>

namespace NeoEngine {

constexpr uint32_t kCapacity = 4096;
constexpr uint32_t kMaxComponentsPerActor = 16;

class ActorComponentWorld {
public:
    bool ValidateStagingCapacity(size_t stagedCount) const noexcept {
        const size_t maxAllowed = static_cast<size_t>(kCapacity) * static_cast<size_t>(kMaxComponentsPerActor);
        if (stagedCount >= maxAllowed) {
            return false;
        }
        return true;
    }
};

} // namespace NeoEngine

#endif // NEO_ACTOR_COMPONENT_WORLD_HPP
