#pragma once
#include "ActorCore.h"

namespace NeoEngine {

class PlayerControllerCore {
public:
    PlayerControllerCore() = default;
    
    void Possess(ActorCore* actor) { m_Pawn = actor; }
    ActorCore* GetPawn() const { return m_Pawn; }
    
    void MoveForward(float amount) {
        if (!m_Pawn) return;
        m_Pawn->Move(0, 0, amount * m_MoveSpeed);
    }
    
    void MoveRight(float amount) {
        if (!m_Pawn) return;
        m_Pawn->Move(amount * m_MoveSpeed, 0, 0);
    }
    
    void Jump() {
        if (!m_Pawn || !m_CanJump) return;
        m_VelocityY = m_JumpForce;
        m_CanJump = false;
    }
    
    void Look(float deltaX, float deltaY) {
        if (!m_Pawn) return;
        auto rot = m_Pawn->GetRotation();
        m_Pawn->SetRotation(rot.x + deltaY * m_LookSensitivity,
                            rot.y + deltaX * m_LookSensitivity, rot.z);
    }
    
    void SetMoveSpeed(float s) { m_MoveSpeed = s; }
    void SetJumpForce(float f) { m_JumpForce = f; }
    
    void Update(float deltaTime) {
        if (!m_Pawn) return;
        // Gravity
        auto pos = m_Pawn->GetPosition();
        if (pos.y > 0) {
            m_VelocityY -= 9.81f * deltaTime;
            m_Pawn->Move(0, m_VelocityY * deltaTime, 0);
        } else {
            m_VelocityY = 0;
            m_CanJump = true;
            m_Pawn->SetPosition(pos.x, 0, pos.z);
        }
    }

private:
    ActorCore* m_Pawn = nullptr;
    float m_MoveSpeed = 5.0f;
    float m_JumpForce = 8.0f;
    float m_LookSensitivity = 0.1f;
    float m_VelocityY = 0;
    bool m_CanJump = true;
};

} // namespace NeoEngine
