#pragma once

#include "../EntityManager.h"
#include "../Components/Transform.h"
#include "../Components/CameraComponent.h"
#include "../../Math/Mat4_Transform.h"

namespace NeoEngine
{

class CameraSystem
{
public:
    Mat4 view;
    Mat4 projection;

    void Update(EntityManager& em)
    {
        auto entities = em.GetEntitiesWith<Transform, CameraComponent>();
        for (auto e : entities)
        {
            auto& t = em.GetComponent<Transform>(e);
            auto& c = em.GetComponent<CameraComponent>(e);
            view       = InverseTransform(t.position, t.rotation);
            projection = Perspective(c.fov, c.aspect, c.nearPlane, c.farPlane);
        }
    }
};

} // namespace NeoEngine
