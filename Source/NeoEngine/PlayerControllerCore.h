#pragma once
#include "ActorCore.h"

namespace NeoEngine {

class PlayerControllerCore {
public:
    PlayerControllerCore() = default;

    void Possess(AActorCore* actor) { m_Pawn = actor; }
    AActorCore* GetPawn() const { return m_Pawn; }

    void MoveForward(float amount) {
        if (!m_Pawn) return;
        m_Pawn->AddWorldOffset(m_Pawn->GetForwardVector() * (amount * m_MoveSpeed));
    }

    void MoveRight(float amount) {
        if (!m_Pawn) return;
        m_Pawn->AddWorldOffset(m_Pawn->GetRightVector() * (amount * m_MoveSpeed));
    }

    void Jump() {
        if (!m_Pawn || !m_CanJump) return;
        m_VelocityY = m_JumpForce;
        m_CanJump = false;
    }

    void Update(float deltaTime) {
        if (!m_Pawn) return;
        m_VelocityY -= 9.81f * deltaTime;
        m_Pawn->AddWorldOffset({0, m_VelocityY * deltaTime, 0});
        if (m_Pawn->GetLocation().y <= 0) {
            m_VelocityY = 0;
            m_CanJump = true;
        }
    }

private:
    AActorCore* m_Pawn = nullptr;
    float m_MoveSpeed = 5.0f;
    float m_JumpForce = 8.0f;
    float m_VelocityY = 0;
    bool m_CanJump = true;
};

} // namespace NeoEngine
