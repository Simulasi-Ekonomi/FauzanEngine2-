#include "Core/Math/NeoMath.h"
#pragma once
#include <string>
#include <vector>
#include <functional>

namespace NeoEngine {

enum class AIBehaviorState {
    Idle, Patrol, Chase, Attack, Flee, Search, Dead
};

class NeoAIComponent {
public:
    NeoAIComponent();
    ~NeoAIComponent() = default;

    // Inisialisasi
    void Initialize(const std::string& ownerName);

    // Update setiap frame
    void Update(float deltaTime);

    // Set posisi target (player atau titik patrol)
    void SetTarget(float x, float y, float z);

    // Set posisi sendiri
    void SetPosition(float x, float y, float z);

    // Getter
    AIBehaviorState GetState() const { return m_State; }
    float GetPosX() const { return m_PosX; }
    float GetPosY() const { return m_PosY; }
    float GetPosZ() const { return m_PosZ; }
    float GetHealth() const { return m_Health; }
    bool IsAlive() const { return m_Health > 0; }

    // Aksi
    void TakeDamage(float damage);

    // Konfigurasi
    void SetAttackRange(float range) { m_AttackRange = range; }
    void SetDetectionRange(float range) { m_DetectionRange = range; }
    void SetMoveSpeed(float speed) { m_MoveSpeed = speed; }

private:
    void UpdateState(float deltaTime);
    void ProcessIdle(float dt);
    void ProcessPatrol(float dt);
    void ProcessChase(float dt);
    void ProcessAttack(float dt);
    void ProcessFlee(float dt);

    float DistanceToTarget() const;

    AIBehaviorState m_State = AIBehaviorState::Idle;
    float m_PosX = 0, m_PosY = 0, m_PosZ = 0;
    float m_TargetX = 0, m_TargetY = 0, m_TargetZ = 0;
    float m_Health = 100.0f;
    float m_MaxHealth = 100.0f;
    float m_AttackRange = 5.0f;
    float m_DetectionRange = 30.0f;
    float m_MoveSpeed = 4.0f;
    float m_AttackCooldown = 1.0f;
    float m_AttackTimer = 0.0f;
    float m_PatrolAngle = 0.0f;
    std::string m_OwnerName;
};

} // namespace NeoEngine
