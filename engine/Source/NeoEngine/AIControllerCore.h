#pragma once
#include <vector>
#include <string>
#include <cmath>

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
    
    void TakeDamage(int dmg) {
        m_Health -= dmg;
        if (m_Health <= 0) { m_Health = 0; m_State = AIState::Dead; }
        else if (m_State != AIState::Attack) m_State = AIState::Chase;
    }
    
    void Update(float deltaTime) {
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
                if (distToPlayer < m_Config.attackRange) {
                    m_State = AIState::Attack;
                } else if (distToPlayer > m_Config.detectionRange * 2) {
                    m_State = AIState::Patrol;
                } else {
                    MoveToward(m_PlayerX, m_PlayerY, m_PlayerZ, deltaTime);
                }
                break;
            case AIState::Attack:
                if (distToPlayer > m_Config.attackRange) m_State = AIState::Chase;
                else if (m_AttackTimer <= 0) {
                    m_AttackTimer = m_Config.attackCooldown;
                }
                break;
            case AIState::Flee:
                MoveAway(m_PlayerX, m_PlayerY, m_PlayerZ, deltaTime);
                if (distToPlayer > m_Config.detectionRange * 3) m_State = AIState::Patrol;
                break;
            default: break;
        }
    }
    
    AIState GetState() const { return m_State; }
    float GetPosX() const { return m_PosX; }
    float GetPosY() const { return m_PosY; }
    float GetPosZ() const { return m_PosZ; }
    int GetHealth() const { return m_Health; }
    
private:
    void MoveToward(float tx, float ty, float tz, float dt) {
        float dx = tx - m_PosX, dy = ty - m_PosY, dz = tz - m_PosZ;
        float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
        if (dist < 0.1f) return;
        float speed = m_Config.moveSpeed * dt;
        m_PosX += (dx / dist) * speed;
        m_PosY += (dy / dist) * speed;
        m_PosZ += (dz / dist) * speed;
    }
    
    void MoveAway(float tx, float ty, float tz, float dt) {
        float dx = m_PosX - tx, dy = m_PosY - ty, dz = m_PosZ - tz;
        float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
        if (dist < 0.1f) return;
        float speed = m_Config.moveSpeed * dt;
        m_PosX += (dx / dist) * speed;
        m_PosY += (dy / dist) * speed;
        m_PosZ += (dz / dist) * speed;
    }
    
    void Patrol(float dt) {
        static float angle = 0;
        angle += dt * 0.5f;
        m_PosX += std::cos(angle) * 0.5f * dt;
        m_PosZ += std::sin(angle) * 0.5f * dt;
    }
    
    float Distance(float x, float y, float z) const {
        float dx = x - m_PosX, dy = y - m_PosY, dz = z - m_PosZ;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }
    
    AIConfig m_Config;
    AIState m_State = AIState::Idle;
    float m_PosX = 0, m_PosY = 0, m_PosZ = 0;
    float m_TargetX = 0, m_TargetY = 0, m_TargetZ = 0;
    float m_PlayerX = 0, m_PlayerY = 0, m_PlayerZ = 0;
    float m_AttackTimer = 0;
    int m_Health = 100;
};

} // namespace NeoEngine
