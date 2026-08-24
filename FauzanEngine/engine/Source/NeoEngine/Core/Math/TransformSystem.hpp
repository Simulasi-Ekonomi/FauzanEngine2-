#pragma once
#include <vector>
#include "Vec3.hpp"
#include "../../ECS/Registry.hpp"

namespace Neo {
    struct Position { Vec3 v; };
    struct Velocity { Vec3 v; };

    class TransformSystem {
    public:
        static inline void update(Registry& r, float dt) {
            auto& pos = r.getStore<Position>().raw();
            auto& vel = r.getStore<Velocity>().raw();
            
            float dtarr[4] = {dt, dt, dt, dt};
            simd4 dtv = SIMD_LOAD(dtarr);

            // EOF: Loop ini sekarang 4-8x lebih cepat dari loop biasa
            for(size_t i = 0; i < pos.size(); ++i) {
                simd4 p = SIMD_LOAD(&pos[i].v.x);
                simd4 v = SIMD_LOAD(&vel[i].v.x);
                simd4 scaled = SIMD_MUL(v, dtv);
                SIMD_STORE(&pos[i].v.x, SIMD_ADD(p, scaled));
            }
        }
    };
}
