#include "AIControllerCore.h"
#include <cmath>

namespace NeoEngine {

void AIControllerCore::TakeDamage(int dmg) {
    m_Health -= dmg;
    if (m_Health <= 0) { m_Health = 0; m_State = AIState::Dead; }
    else if (m_State != AIState::Attack) m_State = AIState::Chase;
}

void AIControllerCore::Update(float deltaTime) {
    if (m_State == AIState::Dead) return;
    m_AttackTimer -= deltaTime;
    float distToPlayer = Distance(m_PlayerX, m_PlayerY, m_PlayerZ);
    
    switch (m_State) {
        case AIState::Idle:
        case AIState::Patrol:
            if (distToPlayer < m_Config.detectionRange) m_State = AIState::Chase;
            else Patrol(deltaTime);
            break;
        case AIState::Chase:
            if (distToPlayer < m_Config.attackRange) m_State = AIState::Attack;
            else if (distToPlayer > m_Config.detectionRange * 2) m_State = AIState::Patrol;
            else MoveToward(m_PlayerX, m_PlayerY, m_PlayerZ, deltaTime);
            break;
        case AIState::Attack:
            if (distToPlayer > m_Config.attackRange) m_State = AIState::Chase;
            break;
        case AIState::Flee:
            MoveAway(m_PlayerX, m_PlayerY, m_PlayerZ, deltaTime);
            if (distToPlayer > m_Config.detectionRange * 3) m_State = AIState::Patrol;
            break;
        default: break;
    }
}

void AIControllerCore::MoveToward(float tx, float ty, float tz, float dt) {
    float dx = tx - m_PosX, dy = ty - m_PosY, dz = tz - m_PosZ;
    float dist = NeoEngine::Math::Sqrt(dx*dx + dy*dy + dz*dz);
    if (dist < 0.1f) return;
    float speed = m_Config.moveSpeed * dt;
    m_PosX += (dx / dist) * speed;
    m_PosZ += (dz / dist) * speed;
}

void AIControllerCore::MoveAway(float tx, float ty, float tz, float dt) {
    float dx = m_PosX - tx, dz = m_PosZ - tz;
    float dist = NeoEngine::Math::Sqrt(dx*dx + dz*dz);
    if (dist < 0.1f) return;
    float speed = m_Config.moveSpeed * dt;
    m_PosX += (dx / dist) * speed;
    m_PosZ += (dz / dist) * speed;
}

void AIControllerCore::Patrol(float dt) {
    static float angle = 0;
    angle += dt * 0.5f;
    m_PosX += NeoEngine::Math::Cos(angle) * 0.5f * dt;
    m_PosZ += NeoEngine::Math::Sin(angle) * 0.5f * dt;
}

float AIControllerCore::Distance(float x, float y, float z) const {
    float dx = x - m_PosX, dy = y - m_PosY, dz = z - m_PosZ;
    return NeoEngine::Math::Sqrt(dx*dx + dy*dy + dz*dz);
}

} // namespace NeoEngine
