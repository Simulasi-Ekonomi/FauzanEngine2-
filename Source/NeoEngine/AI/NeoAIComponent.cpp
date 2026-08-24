#include "NeoAIComponent.h"
#include <android/log.h>

#define LOG_TAG_AI "NeoAIComponent"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_AI, __VA_ARGS__)

namespace NeoEngine {

NeoAIComponent::NeoAIComponent() = default;

void NeoAIComponent::Initialize(const std::string& ownerName) {
    m_OwnerName = ownerName;
    m_State = AIBehaviorState::Idle;
    m_Health = m_MaxHealth;
    LOGI("AI Component initialized for %s", ownerName.c_str());
}

void NeoAIComponent::Update(float deltaTime) {
    if (!IsAlive()) {
        m_State = AIBehaviorState::Dead;
        return;
    }
    UpdateState(deltaTime);
}

void NeoAIComponent::SetTarget(float x, float y, float z) {
    m_TargetX = x; m_TargetY = y; m_TargetZ = z;
}

void NeoAIComponent::SetPosition(float x, float y, float z) {
    m_PosX = x; m_PosY = y; m_PosZ = z;
}

void NeoAIComponent::TakeDamage(float damage) {
    m_Health -= damage;
    if (m_Health <= 0) {
        m_Health = 0;
        m_State = AIBehaviorState::Dead;
    } else if (m_State != AIBehaviorState::Attack && m_State != AIBehaviorState::Flee) {
        m_State = AIBehaviorState::Chase;
    }
}

void NeoAIComponent::UpdateState(float deltaTime) {
    float dist = DistanceToTarget();

    switch (m_State) {
        case AIBehaviorState::Idle:
        case AIBehaviorState::Patrol:
            if (dist < m_DetectionRange) m_State = AIBehaviorState::Chase;
            else ProcessPatrol(deltaTime);
            break;

        case AIBehaviorState::Chase:
            if (dist < m_AttackRange) m_State = AIBehaviorState::Attack;
            else if (dist > m_DetectionRange * 1.5f) m_State = AIBehaviorState::Patrol;
            else ProcessChase(deltaTime);
            break;

        case AIBehaviorState::Attack:
            if (dist > m_AttackRange * 1.2f) m_State = AIBehaviorState::Chase;
            else ProcessAttack(deltaTime);
            break;

        case AIBehaviorState::Flee:
            ProcessFlee(deltaTime);
            if (dist > m_DetectionRange * 2.0f) m_State = AIBehaviorState::Patrol;
            break;

        case AIBehaviorState::Dead:
        default:
            break;
    }
}

void NeoAIComponent::ProcessPatrol(float dt) {
    m_PatrolAngle += dt * 0.5f;
    m_PosX += NeoEngine::Math::Cos(m_PatrolAngle) * m_MoveSpeed * 0.3f * dt;
    m_PosZ += NeoEngine::Math::Sin(m_PatrolAngle) * m_MoveSpeed * 0.3f * dt;
}

void NeoAIComponent::ProcessChase(float dt) {
    float dx = m_TargetX - m_PosX;
    float dz = m_TargetZ - m_PosZ;
    float dist = NeoEngine::Math::Sqrt(dx*dx + dz*dz);
    if (dist < 0.1f) return;

    float speed = m_MoveSpeed * dt;
    m_PosX += (dx / dist) * speed;
    m_PosZ += (dz / dist) * speed;
}

void NeoAIComponent::ProcessAttack(float dt) {
    m_AttackTimer -= dt;
    if (m_AttackTimer <= 0) {
        m_AttackTimer = m_AttackCooldown;
        LOGI("%s attacks! Target at distance %.1f", m_OwnerName.c_str(), DistanceToTarget());
    }
}

void NeoAIComponent::ProcessFlee(float dt) {
    float dx = m_PosX - m_TargetX;
    float dz = m_PosZ - m_TargetZ;
    float dist = NeoEngine::Math::Sqrt(dx*dx + dz*dz);
    if (dist < 0.1f) return;

    float speed = m_MoveSpeed * 1.5f * dt;
    m_PosX += (dx / dist) * speed;
    m_PosZ += (dz / dist) * speed;
}

float NeoAIComponent::DistanceToTarget() const {
    float dx = m_TargetX - m_PosX;
    float dy = m_TargetY - m_PosY;
    float dz = m_TargetZ - m_PosZ;
    return NeoEngine::Math::Sqrt(dx*dx + dy*dy + dz*dz);
}

} // namespace NeoEngine
