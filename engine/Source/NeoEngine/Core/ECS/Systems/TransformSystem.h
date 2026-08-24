#pragma once

#include "Core/ECS/EntityManager.h"
#include "Core/ECS/Components/Transform.h"

namespace NeoEngine
{

class TransformSystem
{
public:
    float rotationSpeed = 0.8f;

    void Update(EntityManager& em, float dt)
    {
        auto view = em.ViewEntities<Transform>();

        for (auto entity : view.Get())
        {
            auto& t = em.GetComponent<Transform>(entity);

            // =========================
            // ROTASI OTOMATIS (SEMENTARA)
            // =========================
            t.rotation.y += dt * rotationSpeed;

            // =========================
            // UPDATE MATRIX (WAJIB)
            // =========================
            t.UpdateMatrix();
        }
    }
};

}
