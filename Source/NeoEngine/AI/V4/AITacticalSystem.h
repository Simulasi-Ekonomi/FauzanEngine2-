#pragma once
#include "Core/ECS/EntityManager.h"
#include "World/SpatialGrid.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace NeoEngine {

enum class AIState : uint8_t { IDLE = 0, PATROL, CHASE, ATTACK, FLEE };
enum class StrategicCommand : uint8_t { NONE = 0, ATTACK, FLEE, PATROL, HOLD };

struct AIBrainComponent {
    AIState state = AIState::IDLE;
    float stateTimer = 0.0f;
    float tacticalCooldown = 0.0f;
    uint32_t targetEntity = 0;
    float patrolAngle = 0.0f;
    float aggression = 0.5f;
    float fear = 0.3f;
    bool hasStrategicPlan = false;
    StrategicCommand strategicCommand = StrategicCommand::NONE;
};

class FauzanAI;

class AITacticalSystem {
public:
    void Init(size_t maxEntities) {
        brains_.resize(maxEntities);
        rngState_ = 123456789;
    }

    void SetAI(FauzanAI* ai) { fauzanAI_ = ai; }

    void Execute(EntityManager& em, float dt) {
        auto entities = em.Query<PositionComponent, VelocityComponent, ColliderComponent>();
        if (entities.empty()) return;

        EntityID maxID = *std::max_element(entities.begin(), entities.end());
        if (brains_.size() <= maxID) brains_.resize(maxID + 1);

        float* px = em.positions.x.data();
        float* pz = em.positions.z.data();
        float* vx = em.velocities.vx.data();
        float* vz = em.velocities.vz.data();

        // Poll strategic commands dari FauzanAI
        if (fauzanAI_) {
            uint32_t respEntity; AICommand cmd;
            while (fauzanAI_->PollAICommand(respEntity, cmd)) {
                if (respEntity < brains_.size() && em.IsAlive(respEntity)) {
                    brains_[respEntity].hasStrategicPlan = true;
                    switch (cmd.commandID) {
                        case 1: brains_[respEntity].strategicCommand = StrategicCommand::ATTACK; break;
                        case 2: brains_[respEntity].strategicCommand = StrategicCommand::FLEE; break;
                        case 3: brains_[respEntity].strategicCommand = StrategicCommand::PATROL; break;
                        case 4: brains_[respEntity].strategicCommand = StrategicCommand::HOLD; break;
                        default: brains_[respEntity].strategicCommand = StrategicCommand::NONE; break;
                    }
                }
            }
        }

        static SpatialGrid grid(10.0f, 5.0f);
        grid.Clear();
        for (EntityID id : entities) {
            if (em.IsAlive(id)) grid.Insert(id, px[id], 0.0f, pz[id]);
        }

        const float halfWorld = 5.0f;
        const float cellSize = 5.0f;
        const int gridWidth = (int)(10.0f / cellSize);
        auto getCellIndex = [&](float x, float z) -> int {
            int cx = (int)((x + halfWorld) / cellSize);
            int cz = (int)((z + halfWorld) / cellSize);
            if (cx < 0) cx = 0; else if (cx >= gridWidth) cx = gridWidth-1;
            if (cz < 0) cz = 0; else if (cz >= gridWidth) cz = gridWidth-1;
            return cz * gridWidth + cx;
        };

        static int batchOffset = 0;
        batchOffset = (batchOffset + 1) % 4;

        int processed = 0;
        const int MAX_PROCESS_PER_FRAME = 2000;

        for (EntityID id : entities) {
            if (!em.IsAlive(id)) continue;
            if ((id % 4) != batchOffset) continue;
            if (processed >= MAX_PROCESS_PER_FRAME) break;

            auto& brain = brains_[id];
            brain.tacticalCooldown -= dt;
            if (brain.tacticalCooldown > 0.0f) continue;
            brain.tacticalCooldown = 0.5f;

            float scoreChase = 0.0f, scoreFlee = 0.0f, scorePatrol = 0.3f, scoreIdle = 0.1f;
            float closestDist2 = 25.0f;
            uint32_t closestEnemy = 0;

            int homeCell = getCellIndex(px[id], pz[id]);
            grid.ForEachPairInCellAndNeighbors(homeCell, [&](EntityID a, EntityID b) {
                if (a != id && b != id) return;
                EntityID other = (a == id) ? b : a;
                if (!em.IsAlive(other)) return;
                float dx = px[id] - px[other];
                float dz = pz[id] - pz[other];
                float d2 = dx*dx + dz*dz;
                if (d2 < closestDist2) {
                    closestDist2 = d2;
                    closestEnemy = other;
                }
            });

            if (closestEnemy != 0 && em.IsAlive(closestEnemy)) {
                float closestDist = sqrtf(closestDist2);
                scoreChase = brain.aggression * (5.0f - closestDist) / 5.0f;
                scoreFlee = brain.fear * (1.0f - (5.0f - closestDist) / 5.0f);
            }

            if (brain.hasStrategicPlan && fauzanAI_) {
                switch (brain.strategicCommand) {
                    case StrategicCommand::ATTACK: scoreChase += 0.5f; break;
                    case StrategicCommand::FLEE:   scoreFlee += 0.5f; break;
                    case StrategicCommand::PATROL: scorePatrol += 0.5f; break;
                    default: break;
                }
                brain.hasStrategicPlan = false;
                brain.strategicCommand = StrategicCommand::NONE;
            }

            AIState newState = AIState::IDLE;
            float bestScore = scoreIdle;
            if (scorePatrol > bestScore) { bestScore = scorePatrol; newState = AIState::PATROL; }
            if (scoreChase > bestScore)  { bestScore = scoreChase;  newState = AIState::CHASE; }
            if (scoreFlee > bestScore)   { bestScore = scoreFlee;   newState = AIState::FLEE; }

            brain.state = newState;
            brain.stateTimer = 0.0f;
            brain.targetEntity = (newState == AIState::CHASE || newState == AIState::FLEE) ? closestEnemy : 0;

            switch (brain.state) {
                case AIState::PATROL:
                    brain.patrolAngle += (int)(xorshift() % 200 - 100) * 0.01f;
                    vx[id] += cosf(brain.patrolAngle) * 0.5f * dt;
                    vz[id] += sinf(brain.patrolAngle) * 0.5f * dt;
                    break;
                case AIState::CHASE:
                    if (closestEnemy != 0 && em.IsAlive(closestEnemy)) {
                        float dx = px[closestEnemy] - px[id];
                        float dz = pz[closestEnemy] - pz[id];
                        float len = sqrtf(dx*dx + dz*dz);
                        if (len > 0.1f) {
                            vx[id] += (dx/len) * 2.0f * dt;
                            vz[id] += (dz/len) * 2.0f * dt;
                        }
                    }
                    break;
                case AIState::FLEE:
                    if (closestEnemy != 0 && em.IsAlive(closestEnemy)) {
                        float dx = px[id] - px[closestEnemy];
                        float dz = pz[id] - pz[closestEnemy];
                        float len = sqrtf(dx*dx + dz*dz);
                        if (len > 0.1f) {
                            vx[id] += (dx/len) * 2.5f * dt;
                            vz[id] += (dz/len) * 2.5f * dt;
                        }
                    }
                    break;
                default: break;
            }

            processed++;
        }
    }

    const AIBrainComponent& GetBrain(EntityID id) const { return brains_[id]; }
    size_t GetBrainCount() const { return brains_.size(); }

private:
    std::vector<AIBrainComponent> brains_;
    FauzanAI* fauzanAI_ = nullptr;
    uint32_t rngState_ = 123456789;

    uint32_t xorshift() {
        rngState_ ^= rngState_ << 13;
        rngState_ ^= rngState_ >> 17;
        rngState_ ^= rngState_ << 5;
        return rngState_;
    }
};

} // namespace NeoEngine
