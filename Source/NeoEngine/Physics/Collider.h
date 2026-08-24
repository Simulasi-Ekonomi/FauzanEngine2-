#include "Core/Math/Vector3.h"
#pragma once
#include "RigidBody.h"

namespace NeoEngine {

class Collider {
public:
    AABB bounds;

    Collider(RigidBody* body) : body_(body) {}

    AABB GetAABB() const {
        AABB box = bounds;
        if (body_) {
            Vec3 pos = body_->GetPosition();
            Vec3 half = bounds.Extents();
            box.min = {pos.x - half.x, pos.y - half.y, pos.z - half.z};
            box.max = {pos.x + half.x, pos.y + half.y, pos.z + half.z};
        }
        return box;
    }

    RigidBody* GetBody() const { return body_; }

private:
    RigidBody* body_ = nullptr;
};

} // namespace NeoEngine
