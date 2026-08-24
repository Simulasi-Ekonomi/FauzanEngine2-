#pragma once
#include "Registry.hpp"

namespace Neo {
    template<typename Derived>
    class System {
    public:
        FORCE_INLINE void update(Registry& reg, float dt) {
            static_cast<Derived*>(this)->updateImpl(reg, dt);
        }
    };
}
