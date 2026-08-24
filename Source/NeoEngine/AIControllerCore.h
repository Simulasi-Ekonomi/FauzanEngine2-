#include "Core/Math/NeoMath.h"
#pragma once
#include <vector>
#include <string>

namespace NeoEngine {

enum class AIState { Idle, Patrol, Chase, Attack, Flee, Dead };

struct AIConfig {
    float patrolRadius = 20.0f;
    float detectionRange = 30.0f;
    float attackRange = 5.0f;
    float moveSpeed = 3.0f;
    float rotationSpeed = 180.0f;
    float attackCooldown = 1.0f;
    int maxHealth = 100;
    int damage = 10;
};

class AIControllerCore {
public:
    AIControllerCore() = default;

    void Initialize(const AIConfig& config) { m_Config = config; m_State = AIState::Idle; m_Health = config.maxHealth; }
    void SetTarget(float tx, float ty, float tz) { m_TargetX = tx; m_TargetY = ty; m_TargetZ = tz; }
    void SetPosition(float px, float py, float pz) { m_PosX = px; m_PosY = py; m_PosZ = pz; }
    void SetPlayerPosition(float px, float py, float pz) { m_PlayerX = px; m_PlayerY = py; m_PlayerZ = pz; }

    void TakeDamage(int dmg);
    void Update(float deltaTime);
    
    AIState GetState() const { return m_State; }
    float GetPosX() const { return m_PosX; }
    float GetPosY() const { return m_PosY; }
    float GetPosZ() const { return m_PosZ; }
    int GetHealth() const { return m_Health; }

private:
    void MoveToward(float tx, float ty, float tz, float dt);
    void MoveAway(float tx, float ty, float tz, float dt);
    void Patrol(float dt);
    float Distance(float x, float y, float z) const;

    AIConfig m_Config;
    AIState m_State = AIState::Idle;
    float m_PosX = 0, m_PosY = 0, m_PosZ = 0;
    float m_TargetX = 0, m_TargetY = 0, m_TargetZ = 0;
    float m_PlayerX = 0, m_PlayerY = 0, m_PlayerZ = 0;
    float m_AttackTimer = 0;
    int m_Health = 100;
};

} // namespace NeoEngine
