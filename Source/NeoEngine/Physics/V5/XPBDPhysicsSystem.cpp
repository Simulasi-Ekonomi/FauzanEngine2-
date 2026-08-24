#include "XPBDPhysicsSystem.h"
#include "Core/ECS/Components.h"
#include "Threading/JobSystem.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <bit>
#include <chrono>


constexpr size_t CONTACT_BUDGET = NeoEngine::MAX_CONTACTS;
namespace NeoEngine {

// SIMD helpers
static inline float32x4_t FastReciprocal4(float32x4_t x) {
    float32x4_t e = vrecpeq_f32(x);
    e = vmulq_f32(e, vrecpsq_f32(x, e));
    return e;
}
static inline float32x4_t FastRsqrt4(float32x4_t x) {
    float32x4_t e = vrsqrteq_f32(x);
    float32x4_t h = vdupq_n_f32(0.5f);
    float32x4_t t = vmulq_f32(vmulq_f32(x, e), e);
    e = vmulq_f32(e, vsubq_f32(vdupq_n_f32(1.5f), vmulq_f32(h, t)));
    return e;
}
inline size_t XPBDPhysicsSystem::HashIndex(uint64_t key) {
    return (key * 11400714819323198485ull) >> STAMP_SHIFT;
}

// Morton buffer now local
static void RadixSortMorton(std::vector<std::pair<uint32_t,int>>& data) {
    std::vector<std::pair<uint32_t,int>> temp(data.size());
    for (int shift = 0; shift <= 22; shift += 11) {
        size_t count[2048] = {0};
        uint32_t mask = 2047 << shift;
        for (auto& p : data) ++count[(p.first & mask) >> shift];
        size_t pos[2048]; pos[0] = 0;
        for (int i = 1; i < 2048; ++i) pos[i] = pos[i-1] + count[i-1];
        for (auto& p : data) temp[pos[(p.first & mask) >> shift]++] = p;
        data.swap(temp);
    }
}

XPBDPhysicsSystem::XPBDPhysicsSystem() {
    m_IsAwake.resize(PHYS_ENTITIES_MAX, 1);
    m_IsAwakePrev.resize(PHYS_ENTITIES_MAX, 1);
    m_flatPosX.resize(PHYS_ENTITIES_MAX, 0.0f);
    m_flatPosZ.resize(PHYS_ENTITIES_MAX, 0.0f);
    m_flatVelX.resize(PHYS_ENTITIES_MAX, 0.0f);
    m_flatVelZ.resize(PHYS_ENTITIES_MAX, 0.0f);
    m_flatRadius.resize(PHYS_ENTITIES_MAX, 0.0f);
    m_flatInvMass.resize(PHYS_ENTITIES_MAX, 0.0f);
    m_flatEntityIDs.resize(PHYS_ENTITIES_MAX, 0);
    m_DeferredDelta.resize(PHYS_ENTITIES_MAX, {0,0,0,0,0,0});
    m_EntityLayers.resize(PHYS_ENTITIES_MAX, COLLISION_LAYER_DEFAULT);
    m_flatRot.resize(PHYS_ENTITIES_MAX, 0.0f);
    m_flatAngVel.resize(PHYS_ENTITIES_MAX, 0.0f);
    m_flatInvInertia.resize(PHYS_ENTITIES_MAX, 1.0f);
    m_EntityColor.resize(PHYS_ENTITIES_MAX, -1);
    m_ConstraintColor.resize(MAX_CONSTRAINTS, -1);
    m_flatPosXPrev.resize(PHYS_ENTITIES_MAX, 0.0f);
    m_flatPosZPrev.resize(PHYS_ENTITIES_MAX, 0.0f);
    m_ChunkAssignment.resize(PHYS_ENTITIES_MAX, -1);
    m_ThreadDeltas.resize(MAX_WORKER_THREADS);
    m_ThreadDeltaActive.resize(MAX_WORKER_THREADS, 0);
    m_MergeJobContexts.resize(MAX_WORKER_THREADS);
    m_PairStamp.resize(STAMP_SIZE, SIZE_MAX);
    m_PairKeys.resize(STAMP_SIZE, 0);
    m_UF_Parent.resize(PHYS_ENTITIES_MAX, 0);
    m_UF_ParentPrev.resize(PHYS_ENTITIES_MAX, 0);
    m_IslandRoots.resize(PHYS_ENTITIES_MAX, 0);
    m_IslandSizes.resize(PHYS_ENTITIES_MAX, 0);
    m_RootToIslandIndex.resize(PHYS_ENTITIES_MAX, 0);
    m_IslandRanges.reserve(MAX_CONTACTS / 64 + 1);
    m_IslandContactBlock.reserve(MAX_CONTACTS);
    m_IslandDirty.reserve(MAX_CONTACTS / 64 + 1);
    m_IslandRootsPrev.reserve(PHYS_ENTITIES_MAX);
    m_IslandSizesPrev.reserve(PHYS_ENTITIES_MAX);
    m_IslandOrder.reserve(MAX_CONTACTS / 64 + 1);
    m_SolveBuffers.resize(MAX_WORKER_THREADS);
    m_Manifolds.resize(MAX_MANIFOLDS);
    m_ManifoldStamp.resize(STAMP_SIZE, UINT32_MAX);
    m_DenseShardManifolds.resize(DENSE_MANIFOLD_TOTAL);
    m_DenseShardStamp.resize(STAMP_SIZE, UINT32_MAX);
    for (auto& contacts : m_DenseShardContacts) contacts.reserve(MAX_CONTACTS / DENSE_MANIFOLD_SHARDS + 1);
    m_BVHStack.reserve(4096);
    m_LeafNode.resize(PHYS_ENTITIES_MAX, -1);
    m_PostOrderCache.reserve(PHYS_ENTITIES_MAX * 2);
    m_PersistentContacts.resize(STAMP_SIZE);
    m_ContactBlocks.resize(MAX_CONTACTS / CONTACT_BLOCK_SIZE + 1);
    m_SortedContactIndices.reserve(MAX_CONTACTS);
    m_ColorBatches.reserve(MAX_GRAPH_COLORS);
    m_GridContactCandidates.resize(MAX_WORKER_THREADS);
    m_GridCandidatePairCounts.resize(MAX_WORKER_THREADS, 0);
    m_GridGatherContexts.resize(MAX_WORKER_THREADS);
    m_GridStageContexts.resize(MAX_WORKER_THREADS);
    for (auto& candidates : m_GridContactCandidates) candidates.reserve(MAX_CONTACTS / MAX_WORKER_THREADS + 1);
    m_Constraints.resize(MAX_CONSTRAINTS);
    m_maxFlatEntities = PHYS_ENTITIES_MAX;
    m_PairStamp.resize(STAMP_SIZE, SIZE_MAX);
    m_PairKeys.resize(STAMP_SIZE, 0);
    m_IsAwake.resize(PHYS_ENTITIES_MAX, 0);
    m_IsAwakePrev.resize(PHYS_ENTITIES_MAX, 0);
}

// Constraint API
uint32_t XPBDPhysicsSystem::AddHingeJoint(uint32_t idxA, uint32_t idxB,
    float anchorAX, float anchorAZ, float anchorBX, float anchorBZ,
    float axisX, float axisZ, float motorSpeed, float breakThreshold) {
    if (m_ConstraintCount >= MAX_CONSTRAINTS) return UINT32_MAX;
    uint32_t id = m_ConstraintCount++;
    Constraint& c = m_Constraints[id];
    c.idxA = idxA; c.idxB = idxB;
    c.type = (motorSpeed != 0.0f) ? ConstraintType::MotorizedHinge : ConstraintType::Hinge;
    c.anchorAX = anchorAX; c.anchorAZ = anchorAZ;
    c.anchorBX = anchorBX; c.anchorBZ = anchorBZ;
    c.axisX = axisX; c.axisZ = axisZ;
    c.lambda[0] = c.lambda[1] = c.lambda[2] = 0.0f;
    c.motorSpeed = motorSpeed; c.breakThreshold = breakThreshold;
    c.stiffness = 1000.0f; c.maxForce = 0.0f;
    c.limitLow = c.limitHigh = 0.0f;
    c.age = 0; c.active = true;
    return id;
}
uint32_t XPBDPhysicsSystem::AddConeTwistJoint(uint32_t idxA, uint32_t idxB,
    float anchorAX, float anchorAZ, float anchorBX, float anchorBZ,
    float limitLow, float limitHigh) {
    if (m_ConstraintCount >= MAX_CONSTRAINTS) return UINT32_MAX;
    uint32_t id = m_ConstraintCount++;
    Constraint& c = m_Constraints[id];
    c.idxA = idxA; c.idxB = idxB; c.type = ConstraintType::ConeTwist;
    c.anchorAX = anchorAX; c.anchorAZ = anchorAZ;
    c.anchorBX = anchorBX; c.anchorBZ = anchorBZ;
    c.lambda[0]=c.lambda[1]=c.lambda[2]=0.0f;
    c.motorSpeed=0.0f; c.breakThreshold=0.0f;
    c.limitLow=limitLow; c.limitHigh=limitHigh;
    c.age=0; c.active=true;
    return id;
}
uint32_t XPBDPhysicsSystem::AddPrismaticJoint(uint32_t idxA, uint32_t idxB,
    float axisX, float axisZ, float limitLow, float limitHigh) {
    if (m_ConstraintCount >= MAX_CONSTRAINTS) return UINT32_MAX;
    uint32_t id = m_ConstraintCount++;
    Constraint& c = m_Constraints[id];
    c.idxA=idxA; c.idxB=idxB; c.type=ConstraintType::Prismatic;
    c.anchorAX=c.anchorAZ=0; c.anchorBX=c.anchorBZ=0;
    c.axisX=axisX; c.axisZ=axisZ;
    c.lambda[0]=c.lambda[1]=c.lambda[2]=0.0f;
    c.motorSpeed=0.0f; c.breakThreshold=0.0f;
    c.limitLow=limitLow; c.limitHigh=limitHigh;
    c.age=0; c.active=true;
    return id;
}
void XPBDPhysicsSystem::RemoveConstraint(uint32_t id) { if (id < m_ConstraintCount) m_Constraints[id].active = false; }

uint32_t XPBDPhysicsSystem::AddDistanceJoint(uint32_t a, uint32_t b, float minDist, float maxDist, float stiffness) {
    if (m_ConstraintCount >= MAX_CONSTRAINTS || a == b || minDist < 0.0f || maxDist < minDist) return UINT32_MAX;
    Constraint& constraint = m_Constraints[m_ConstraintCount];
    constraint = {};
    constraint.idxA = a; constraint.idxB = b; constraint.type = ConstraintType::Distance;
    constraint.limitLow = minDist; constraint.limitHigh = maxDist;
    constraint.stiffness = std::max(stiffness, 1.0f); constraint.active = true;
    return static_cast<uint32_t>(m_ConstraintCount++);
}

uint32_t XPBDPhysicsSystem::AddFixedJoint(uint32_t a, uint32_t b, float anchorAX, float anchorAZ,
                                          float anchorBX, float anchorBZ) {
    if (m_ConstraintCount >= MAX_CONSTRAINTS || a == b) return UINT32_MAX;
    Constraint& constraint = m_Constraints[m_ConstraintCount];
    constraint = {};
    constraint.idxA = a; constraint.idxB = b; constraint.type = ConstraintType::Fixed;
    constraint.anchorAX = anchorAX; constraint.anchorAZ = anchorAZ;
    constraint.anchorBX = anchorBX; constraint.anchorBZ = anchorBZ;
    constraint.stiffness = 10000.0f; constraint.active = true;
    return static_cast<uint32_t>(m_ConstraintCount++);
}

uint32_t XPBDPhysicsSystem::AddSphericalJoint(uint32_t a, uint32_t b, float anchorAX, float anchorAZ,
                                              float anchorBX, float anchorBZ, float limitLow, float limitHigh) {
    if (m_ConstraintCount >= MAX_CONSTRAINTS || a == b || limitHigh < limitLow) return UINT32_MAX;
    Constraint& constraint = m_Constraints[m_ConstraintCount];
    constraint = {};
    constraint.idxA = a; constraint.idxB = b; constraint.type = ConstraintType::Spherical;
    constraint.anchorAX = anchorAX; constraint.anchorAZ = anchorAZ;
    constraint.anchorBX = anchorBX; constraint.anchorBZ = anchorBZ;
    constraint.limitLow = limitLow; constraint.limitHigh = limitHigh;
    constraint.stiffness = 5000.0f; constraint.active = true;
    return static_cast<uint32_t>(m_ConstraintCount++);
}

void XPBDPhysicsSystem::SetConstraintDrive(uint32_t id, float targetVelocity, float maxForce) {
    if (id >= m_ConstraintCount || maxForce < 0.0f) return;
    Constraint& constraint = m_Constraints[id];
    constraint.motorSpeed = targetVelocity;
    constraint.maxForce = maxForce;
    if (constraint.type == ConstraintType::Hinge && maxForce > 0.0f) constraint.type = ConstraintType::MotorizedHinge;
}

// BuildFlatArrays
void XPBDPhysicsSystem::BuildFlatArrays(ArchetypeManager& em) {
    const uint64_t ecsRevision = em.GetPhysicsRevision();
    if (m_FlatArraysValid && m_EcsPhysicsRevision == ecsRevision) return;
    auto chunks = em.GetChunks<PositionComponent, VelocityComponent, ColliderComponent>();
    size_t totalEntities = 0;
    for (auto* chunk : chunks) {
        if (chunk && chunk->count > 0 && chunk->posX != nullptr) totalEntities += chunk->count;
    }
    if (totalEntities == 0) {
        m_activeFlatEntities = 0;
        m_EcsPhysicsRevision = ecsRevision;
        m_FlatArraysValid = true;
        return;
    }
    if (totalEntities > m_maxFlatEntities) {
        m_flatPosX.resize(totalEntities, 0.0f); m_flatPosZ.resize(totalEntities, 0.0f);
        m_flatVelX.resize(totalEntities, 0.0f); m_flatVelZ.resize(totalEntities, 0.0f);
        m_flatRadius.resize(totalEntities, 0.0f); m_flatInvMass.resize(totalEntities, 0.0f);
        m_flatEntityIDs.resize(totalEntities, 0);
        m_DeferredDelta.resize(totalEntities, {0,0,0,0,0,0});
        m_EntityLayers.resize(totalEntities, COLLISION_LAYER_DEFAULT);
        m_flatRot.resize(totalEntities, 0.0f); m_flatAngVel.resize(totalEntities, 0.0f);
        m_flatInvInertia.resize(totalEntities, 1.0f);
        m_EntityColor.resize(totalEntities, -1);
        m_IsAwake.resize(totalEntities, 0);
        m_IsAwakePrev.resize(totalEntities, 0);
m_UF_Parent.resize(totalEntities, 0);
m_UF_ParentPrev.resize(totalEntities, 0);
        m_PairStamp.resize(STAMP_SIZE, SIZE_MAX);
        m_PairKeys.resize(STAMP_SIZE, 0);
        m_flatPosXPrev.resize(totalEntities, 0.0f); m_flatPosZPrev.resize(totalEntities, 0.0f);
        m_ChunkAssignment.resize(totalEntities, -1);
        m_UF_Parent.resize(totalEntities, 0); m_UF_ParentPrev.resize(totalEntities, 0);
        m_RootToIslandIndex.resize(totalEntities, 0);
        m_LeafNode.assign(totalEntities, -1);
        m_maxFlatEntities = totalEntities;
        for (auto& td : m_ThreadDeltas) td.resize(totalEntities, {0,0,0,0,0,0});
    }
    size_t idx = 0;
    for (auto* chunk : chunks) {
        if (!chunk || chunk->count == 0 || chunk->posX == nullptr) continue;
        for (size_t i = 0; i < chunk->count && idx < m_maxFlatEntities; ++i) {
            EntityID id = chunk->entities[i];
            m_flatPosX[idx] = chunk->posX[i]; m_flatPosZ[idx] = chunk->posZ[i];
            m_flatVelX[idx] = chunk->velX[i]; m_flatVelZ[idx] = chunk->velZ[i];
            m_flatRadius[idx] = chunk->radius[i]; m_flatInvMass[idx] = chunk->invMass[i];
            m_flatEntityIDs[idx] = id;
            m_flatRot[idx] = chunk->rotZ ? chunk->rotZ[i] : 0.0f;
            m_flatAngVel[idx] = 0.0f;
            m_flatInvInertia[idx] = (chunk->invMass[i] > 0)
                ? (2.0f / (chunk->radius[i] * chunk->radius[i] * chunk->invMass[i])) : 0.0f;
            ++idx;
        }
    }
    m_activeFlatEntities = idx;
    std::copy(m_flatPosX.begin(), m_flatPosX.begin() + idx, m_flatPosXPrev.begin());
    std::copy(m_flatPosZ.begin(), m_flatPosZ.begin() + idx, m_flatPosZPrev.begin());
    m_EcsPhysicsRevision = ecsRevision;
    m_FlatArraysValid = true;
}

// BVH
void XPBDPhysicsSystem::BuildBVHMorton(std::vector<int>& indices) {
    if (indices.empty()) { m_BVHRoot = -1; return; }
    float minX = 1e30f, minZ = 1e30f, maxX = -1e30f, maxZ = -1e30f;
    for (int e : indices) {
        if (e < 0 || e >= (int)m_activeFlatEntities) continue;
        float cx = m_flatPosX[e], cz = m_flatPosZ[e];
        if (cx < minX) minX = cx; if (cx > maxX) maxX = cx;
        if (cz < minZ) minZ = cz; if (cz > maxZ) maxZ = cz;
    }
    float range = std::max(maxX - minX, maxZ - minZ);
    if (range < 1e-8f) range = 1.0f;
    float invRange = 1.0f / range;
    auto spread = [](uint32_t v) -> uint32_t {
        v = (v | (v << 16)) & 0x030000FF; v = (v | (v << 8)) & 0x0300F00F;
        v = (v | (v << 4)) & 0x030C30C3; v = (v | (v << 2)) & 0x09249249;
        return v;
    };
    std::vector<std::pair<uint32_t,int>> mortonBuffer(indices.size());
    size_t validCount = 0;
    for (size_t i = 0; i < indices.size(); ++i) {
        int e = indices[i];
        if (e < 0 || e >= (int)m_activeFlatEntities) continue;
        uint32_t ix = (uint32_t)(std::min(std::max((m_flatPosX[e]-minX)*invRange,0.0f),1.0f)*1023.0f);
        uint32_t iz = (uint32_t)(std::min(std::max((m_flatPosZ[e]-minZ)*invRange,0.0f),1.0f)*1023.0f);
        mortonBuffer[validCount++] = {spread(ix) | (spread(iz) << 1), e};
    }
    if (validCount == 0) { m_BVHRoot = -1; return; }
    RadixSortMorton(mortonBuffer);
    std::vector<int> sorted(validCount);
    for (size_t i = 0; i < validCount; ++i) sorted[i] = mortonBuffer[i].second;

    m_BVHNodes.clear();
    m_LeafNode.assign(m_activeFlatEntities, -1);
    struct Task { int s, e, p; bool r; };
    std::vector<Task> stk; stk.push_back({0,(int)sorted.size(),-1,false});
    while (!stk.empty()) {
        auto t = stk.back(); stk.pop_back();
        int nid = (int)m_BVHNodes.size(); m_BVHNodes.push_back({});
        m_BVHNodes[nid].parent = t.p;
        if (t.p >= 0) { if (t.r) m_BVHNodes[t.p].right = nid; else m_BVHNodes[t.p].left = nid; }
        else m_BVHRoot = nid;
        if (t.e - t.s == 1) {
            int e = sorted[t.s];
            BVHNode& leaf = m_BVHNodes[nid];
            leaf.isLeaf = true; leaf.entityIdx = e; leaf.left = leaf.right = -1;
            float r = m_flatRadius[e] + FAT_MARGIN;
            leaf.minX = m_flatPosX[e] - r; leaf.maxX = m_flatPosX[e] + r;
            leaf.minZ = m_flatPosZ[e] - r; leaf.maxZ = m_flatPosZ[e] + r;
            leaf.cachedCost = (leaf.maxX - leaf.minX) * (leaf.maxZ - leaf.minZ);
            m_LeafNode[e] = nid;
            continue;
        }
        float nX=1e30f,nZ=1e30f,xX=-1e30f,xZ=-1e30f;
        for (int i = t.s; i < t.e; ++i) {
            int e = sorted[i]; float r = m_flatRadius[e] + FAT_MARGIN;
            float ex = m_flatPosX[e], ez = m_flatPosZ[e];
            if (ex - r < nX) nX = ex - r; if (ex + r > xX) xX = ex + r;
            if (ez - r < nZ) nZ = ez - r; if (ez + r > xZ) xZ = ez + r;
        }
        BVHNode& nd = m_BVHNodes[nid];
        nd.isLeaf = false; nd.minX = nX; nd.maxX = xX; nd.minZ = nZ; nd.maxZ = xZ;
        nd.cachedCost = (xX - nX) * (xZ - nZ);
        int mid = (t.s + t.e) / 2;
        stk.push_back({mid, t.e, nid, true});
        stk.push_back({t.s, mid, nid, false});
    }
    m_BVHInitialized = true;
}
void XPBDPhysicsSystem::BuildBVHIterative(std::vector<int>& indices) { BuildBVHMorton(indices); }
void XPBDPhysicsSystem::RefitNode(int nodeIdx) {
    if (nodeIdx < 0 || nodeIdx >= (int)m_BVHNodes.size()) return;
    BVHNode& node = m_BVHNodes[nodeIdx];
    if (node.isLeaf) {
        if (node.entityIdx < 0 || node.entityIdx >= (int)m_activeFlatEntities) return;
        float r = m_flatRadius[node.entityIdx] + FAT_MARGIN;
        node.minX = m_flatPosX[node.entityIdx] - r; node.maxX = m_flatPosX[node.entityIdx] + r;
        node.minZ = m_flatPosZ[node.entityIdx] - r; node.maxZ = m_flatPosZ[node.entityIdx] + r;
        node.cachedCost = (node.maxX - node.minX) * (node.maxZ - node.minZ);
    } else {
        if (node.left >= 0 && node.right >= 0) {
            BVHNode& l = m_BVHNodes[node.left], &r = m_BVHNodes[node.right];
            node.minX = std::min(l.minX, r.minX); node.minZ = std::min(l.minZ, r.minZ);
            node.maxX = std::max(l.maxX, r.maxX); node.maxZ = std::max(l.maxZ, r.maxZ);
            node.cachedCost = (node.maxX - node.minX) * (node.maxZ - node.minZ) + l.cachedCost + r.cachedCost;
        }
    }
}
void XPBDPhysicsSystem::RefitBVH() {
    if (m_BVHRoot < 0) return;
    for (size_t i = 0; i < m_activeFlatEntities; ++i) {
        if (m_LeafNode[i] == -1) continue;
        float dx = m_flatPosX[i] - m_flatPosXPrev[i];
        float dz = m_flatPosZ[i] - m_flatPosZPrev[i];
        if (fabsf(dx) > FAT_MARGIN || fabsf(dz) > FAT_MARGIN) RefitNode(m_LeafNode[i]);
    }
    std::copy(m_flatPosX.begin(), m_flatPosX.begin() + m_activeFlatEntities, m_flatPosXPrev.begin());
    std::copy(m_flatPosZ.begin(), m_flatPosZ.begin() + m_activeFlatEntities, m_flatPosZPrev.begin());
    m_PostOrderCache.clear();
    m_BVHStack.clear(); m_BVHStack.push_back(m_BVHRoot);
    while (!m_BVHStack.empty()) {
        int n = m_BVHStack.back(); m_BVHStack.pop_back();
        if (n < 0 || n >= (int)m_BVHNodes.size()) continue;
        m_PostOrderCache.push_back(n);
        if (!m_BVHNodes[n].isLeaf) { m_BVHStack.push_back(m_BVHNodes[n].left); m_BVHStack.push_back(m_BVHNodes[n].right); }
    }
    for (int i = (int)m_PostOrderCache.size() - 1; i >= 0; --i) RefitNode(m_PostOrderCache[i]);
}
void XPBDPhysicsSystem::RotateBVH(int nodeIdx) {
    if (nodeIdx < 0 || nodeIdx >= (int)m_BVHNodes.size()) return;
    BVHNode& node = m_BVHNodes[nodeIdx];
    if (node.isLeaf) return;
    float lc = m_BVHNodes[node.left].cachedCost, rc = m_BVHNodes[node.right].cachedCost;
    if (lc + rc < 1e-10f) return;
    if (fabsf(lc - rc) > (lc + rc) * BVH_ROTATE_COST_RATIO) {
        if (lc > rc && !m_BVHNodes[node.left].isLeaf) {
            int oldL = node.left, newL = m_BVHNodes[oldL].right;
            m_BVHNodes[node.left].right = node.right; m_BVHNodes[node.right].parent = node.left;
            m_BVHNodes[newL].parent = nodeIdx;
            node.left = newL; node.right = oldL; m_BVHNodes[oldL].parent = nodeIdx;
        } else if (rc > lc && !m_BVHNodes[node.right].isLeaf) {
            int oldR = node.right, newR = m_BVHNodes[oldR].left;
            m_BVHNodes[node.right].left = node.left; m_BVHNodes[node.left].parent = node.right;
            m_BVHNodes[newR].parent = nodeIdx;
            node.right = newR; node.left = oldR; m_BVHNodes[oldR].parent = nodeIdx;
        }
        int cur = nodeIdx;
        while (cur != -1 && cur < (int)m_BVHNodes.size()) { RefitNode(cur); cur = m_BVHNodes[cur].parent; }
    }
}

// Broadphase & CCD
void XPBDPhysicsSystem::QueryBVHPairsIterative(int rootA, int rootB) {
    if (rootA < 0 || rootB < 0) return;
    m_BVHStack.clear(); m_BVHStack.push_back(rootA); m_BVHStack.push_back(rootB);
    while (!m_BVHStack.empty()) {
        int nodeB = m_BVHStack.back(); m_BVHStack.pop_back();
        int nodeA = m_BVHStack.back(); m_BVHStack.pop_back();
        if (nodeA < 0 || nodeB < 0 || nodeA >= (int)m_BVHNodes.size() || nodeB >= (int)m_BVHNodes.size()) continue;
        if (nodeA > nodeB) std::swap(nodeA, nodeB);
        BVHNode& a = m_BVHNodes[nodeA], &b = m_BVHNodes[nodeB];
        if (a.maxX < b.minX || a.minX > b.maxX || a.maxZ < b.minZ || a.minZ > b.maxZ) continue;
        if (a.isLeaf && b.isLeaf) {
            if (a.entityIdx < 0 || b.entityIdx < 0) continue;
            if (a.entityIdx >= (int)m_activeFlatEntities || b.entityIdx >= (int)m_activeFlatEntities) continue;
            if (a.entityIdx == b.entityIdx) continue;
            if ((m_EntityLayers[a.entityIdx] & m_EntityLayers[b.entityIdx]) == 0) continue;
            uint32_t ia = (uint32_t)a.entityIdx, ib = (uint32_t)b.entityIdx;
            float dx = m_flatPosX[ia] - m_flatPosX[ib];
            float dz = m_flatPosZ[ia] - m_flatPosZ[ib];
            if (ia > ib) {
                std::swap(ia, ib);
                dx = -dx;
                dz = -dz;
            }
            float sumR = m_flatRadius[ia] + m_flatRadius[ib];
            if (fabsf(dx) > sumR || fabsf(dz) > sumR) continue;
            float d2 = dx*dx + dz*dz;
            if (d2 >= sumR*sumR || d2 <= 0.0001f) continue;
            EmitContact(ia, ib, dx, dz, d2, sumR, 1.0f/sqrtf(d2));
            continue;
        }
        if (!a.isLeaf && !b.isLeaf) {
            int aL = a.left, aR = a.right, bL = b.left, bR = b.right;
            if (m_BVHNodes[aL].maxX >= m_BVHNodes[bL].minX && m_BVHNodes[aL].minX <= m_BVHNodes[bL].maxX &&
                m_BVHNodes[aL].maxZ >= m_BVHNodes[bL].minZ && m_BVHNodes[aL].minZ <= m_BVHNodes[bL].maxZ)
            { m_BVHStack.push_back(aL); m_BVHStack.push_back(bL); }
            if (m_BVHNodes[aL].maxX >= m_BVHNodes[bR].minX && m_BVHNodes[aL].minX <= m_BVHNodes[bR].maxX &&
                m_BVHNodes[aL].maxZ >= m_BVHNodes[bR].minZ && m_BVHNodes[aL].minZ <= m_BVHNodes[bR].maxZ)
            { m_BVHStack.push_back(aL); m_BVHStack.push_back(bR); }
            if (m_BVHNodes[aR].maxX >= m_BVHNodes[bL].minX && m_BVHNodes[aR].minX <= m_BVHNodes[bL].maxX &&
                m_BVHNodes[aR].maxZ >= m_BVHNodes[bL].minZ && m_BVHNodes[aR].minZ <= m_BVHNodes[bL].maxZ)
            { m_BVHStack.push_back(aR); m_BVHStack.push_back(bL); }
            if (m_BVHNodes[aR].maxX >= m_BVHNodes[bR].minX && m_BVHNodes[aR].minX <= m_BVHNodes[bR].maxX &&
                m_BVHNodes[aR].maxZ >= m_BVHNodes[bR].minZ && m_BVHNodes[aR].minZ <= m_BVHNodes[bR].maxZ)
            { m_BVHStack.push_back(aR); m_BVHStack.push_back(bR); }
        } else if (a.isLeaf) {
            if (m_BVHNodes[nodeA].maxX >= m_BVHNodes[b.left].minX && m_BVHNodes[nodeA].minX <= m_BVHNodes[b.left].maxX &&
                m_BVHNodes[nodeA].maxZ >= m_BVHNodes[b.left].minZ && m_BVHNodes[nodeA].minZ <= m_BVHNodes[b.left].maxZ)
            { m_BVHStack.push_back(nodeA); m_BVHStack.push_back(b.left); }
            if (m_BVHNodes[nodeA].maxX >= m_BVHNodes[b.right].minX && m_BVHNodes[nodeA].minX <= m_BVHNodes[b.right].maxX &&
                m_BVHNodes[nodeA].maxZ >= m_BVHNodes[b.right].minZ && m_BVHNodes[nodeA].minZ <= m_BVHNodes[b.right].maxZ)
            { m_BVHStack.push_back(nodeA); m_BVHStack.push_back(b.right); }
        } else {
            if (m_BVHNodes[a.left].maxX >= m_BVHNodes[nodeB].minX && m_BVHNodes[a.left].minX <= m_BVHNodes[nodeB].maxX &&
                m_BVHNodes[a.left].maxZ >= m_BVHNodes[nodeB].minZ && m_BVHNodes[a.left].minZ <= m_BVHNodes[nodeB].maxZ)
            { m_BVHStack.push_back(a.left); m_BVHStack.push_back(nodeB); }
            if (m_BVHNodes[a.right].maxX >= m_BVHNodes[nodeB].minX && m_BVHNodes[a.right].minX <= m_BVHNodes[nodeB].maxX &&
                m_BVHNodes[a.right].maxZ >= m_BVHNodes[nodeB].minZ && m_BVHNodes[a.right].minZ <= m_BVHNodes[nodeB].maxZ)
            { m_BVHStack.push_back(a.right); m_BVHStack.push_back(nodeB); }
        }
    }
}

void XPBDPhysicsSystem::GridBroadphase() {
    if (m_activeFlatEntities == 0 || m_GridCellSize <= 0.0f) return;
    using Clock = std::chrono::steady_clock;
    const auto millisSince = [](const Clock::time_point& start) {
        return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
    };
    const auto boundsStarted = m_TimingEnabled ? Clock::now() : Clock::time_point{};

    float minWorldX = INFINITY, minWorldZ = INFINITY;
    float maxWorldX = -INFINITY, maxWorldZ = -INFINITY;
    float maxRadius = 0.0f;
    for (uint32_t index = 0; index < m_activeFlatEntities; ++index) {
        if (m_flatRadius[index] <= 0.0f) continue;
        minWorldX = std::min(minWorldX, m_flatPosX[index]);
        minWorldZ = std::min(minWorldZ, m_flatPosZ[index]);
        maxWorldX = std::max(maxWorldX, m_flatPosX[index]);
        maxWorldZ = std::max(maxWorldZ, m_flatPosZ[index]);
        maxRadius = std::max(maxRadius, m_flatRadius[index]);
    }
    if (maxRadius <= 0.0f) return;
    if (m_TimingEnabled) m_BroadphaseTimingStats.boundsMs = millisSince(boundsStarted);

    const float denseCellSize = std::max(maxRadius * 2.0f * DENSE_GRID_CELL_DIAMETER_SCALE + 0.001f, 0.05f);
    const int minCellX = static_cast<int>(floorf(minWorldX / denseCellSize));
    const int minCellZ = static_cast<int>(floorf(minWorldZ / denseCellSize));
    const int maxCellX = static_cast<int>(floorf(maxWorldX / denseCellSize));
    const int maxCellZ = static_cast<int>(floorf(maxWorldZ / denseCellSize));
    const int64_t width64 = static_cast<int64_t>(maxCellX) - minCellX + 1;
    const int64_t height64 = static_cast<int64_t>(maxCellZ) - minCellZ + 1;
    const size_t denseCellCount = width64 > 0 && height64 > 0
        ? static_cast<size_t>(width64 * height64) : 0;
    const size_t denseCellLimit = std::min<size_t>(2000000, std::max<size_t>(4096, m_activeFlatEntities * 16));

    const auto testPair = [this](uint32_t a, uint32_t b) {
        if (a >= m_activeFlatEntities || b >= m_activeFlatEntities || a == b) return;
        if (!m_IsAwake[a] && !m_IsAwake[b]) return;
        if ((m_EntityLayers[a] & m_EntityLayers[b]) == 0) return;
        ++m_BroadphaseStats.candidatePairs;
        float dx = m_flatPosX[a] - m_flatPosX[b];
        float dz = m_flatPosZ[a] - m_flatPosZ[b];
        if (a > b) { std::swap(a, b); dx = -dx; dz = -dz; }
        const float radius = m_flatRadius[a] + m_flatRadius[b];
        if (fabsf(dx) > radius || fabsf(dz) > radius) return;
        const float distanceSquared = dx * dx + dz * dz;
        if (distanceSquared <= 0.0001f || distanceSquared >= radius * radius) return;
        EmitContact(a, b, dx, dz, distanceSquared, radius, 1.0f / sqrtf(distanceSquared), true);
    };

    if (denseCellCount > 0 && denseCellCount <= denseCellLimit) {
        const auto gridBuildStarted = m_TimingEnabled ? Clock::now() : Clock::time_point{};
        const int width = static_cast<int>(width64);
        const int height = static_cast<int>(height64);
        m_DenseGridHeads.assign(denseCellCount, -1);
        m_DenseGridNext.resize(m_activeFlatEntities);
        m_DenseGridActiveCells.assign(denseCellCount, 0);
        m_DenseGridActiveCellList.clear();
        m_DenseGridActiveCellList.reserve(std::min(denseCellCount, m_activeFlatEntities));
        const auto& heads = m_DenseGridHeads;
        const auto& next = m_DenseGridNext;
        const auto& activeCells = m_DenseGridActiveCells;
        const auto& activeCellList = m_DenseGridActiveCellList;
        const auto denseIndex = [minCellX, minCellZ, width](int x, int z) {
            return static_cast<size_t>(z - minCellZ) * width + static_cast<size_t>(x - minCellX);
        };
        for (uint32_t index = 0; index < m_activeFlatEntities; ++index) {
            if (m_flatRadius[index] <= 0.0f) continue;
            const int cellX = static_cast<int>(floorf(m_flatPosX[index] / denseCellSize));
            const int cellZ = static_cast<int>(floorf(m_flatPosZ[index] / denseCellSize));
            const size_t cell = denseIndex(cellX, cellZ);
            if (m_DenseGridHeads[cell] < 0) ++m_BroadphaseStats.occupiedCells;
            m_DenseGridNext[index] = m_DenseGridHeads[cell];
            m_DenseGridHeads[cell] = static_cast<int>(index);
            if (m_IsAwake[index] && !activeCells[cell]) {
                m_DenseGridActiveCells[cell] = 1;
                m_DenseGridActiveCellList.push_back(cell);
            }
        }
        m_BroadphaseStats.activeCells = m_DenseGridActiveCellList.size();
        if (m_TimingEnabled) m_BroadphaseTimingStats.gridBuildMs = millisSince(gridBuildStarted);
        const auto pairTraversalStarted = m_TimingEnabled ? Clock::now() : Clock::time_point{};
        const size_t workerCount = std::min(std::min(std::max(JobSystem::Get().NumWorkers(), 1UL), MAX_WORKER_THREADS), activeCellList.size());
        const auto gatherStarted = m_TimingEnabled ? Clock::now() : Clock::time_point{};
        for (size_t worker = 0; worker < workerCount; ++worker) {
            const size_t begin = activeCellList.size() * worker / workerCount;
            const size_t end = activeCellList.size() * (worker + 1) / workerCount;
            m_GridGatherContexts[worker] = {this, begin, end, width, height, minCellX, minCellZ, worker};
            JobSystem::Get().ExecuteRaw(&XPBDPhysicsSystem::RunGridGatherJob, &m_GridGatherContexts[worker]);
        }
        JobSystem::Get().WaitForAll();
        if (m_TimingEnabled) m_BroadphaseTimingStats.gatherMs = millisSince(gatherStarted);
        const auto emitStarted = m_TimingEnabled ? Clock::now() : Clock::time_point{};
        size_t stagedContactCount = 0;
        bool canStagePayloads = true;
        for (size_t worker = 0; worker < workerCount; ++worker) {
            m_BroadphaseStats.candidatePairs += m_GridCandidatePairCounts[worker];
            const size_t candidateCount = m_GridContactCandidates[worker].size();
            if (candidateCount > CONTACT_BUDGET - stagedContactCount) {
                canStagePayloads = false;
                break;
            }
            m_GridStageContexts[worker] = {this, worker, stagedContactCount};
            stagedContactCount += candidateCount;
        }
        if (canStagePayloads) {
            for (size_t worker = 0; worker < workerCount; ++worker)
                JobSystem::Get().ExecuteRaw(&XPBDPhysicsSystem::RunGridStageJob, &m_GridStageContexts[worker]);
            JobSystem::Get().WaitForAll();
            m_ContactCount = stagedContactCount;
            AssociateStagedDenseGridManifolds(workerCount);
        } else {
            // Capacity risk retains the exact previous fail-closed serial path.
            for (size_t worker = 0; worker < workerCount; ++worker) {
                for (const GridContactCandidate& candidate : m_GridContactCandidates[worker])
                    EmitContact(candidate.idxA, candidate.idxB, candidate.dx, candidate.dz, candidate.d2,
                                candidate.sumR, 1.0f / sqrtf(candidate.d2), true, true);
            }
        }
        if (m_TimingEnabled) m_BroadphaseTimingStats.emitMs = millisSince(emitStarted);
        if (m_TimingEnabled) m_BroadphaseTimingStats.pairTraversalMs = millisSince(pairTraversalStarted);
        return;
    }

    std::vector<std::pair<uint64_t, uint32_t>> entries;
    entries.reserve(m_activeFlatEntities * 2);
    for (uint32_t index = 0; index < m_activeFlatEntities; ++index) {
        if (m_flatRadius[index] <= 0.0f) continue;
        const float radius = m_flatRadius[index];
        const int minX = static_cast<int>(floorf((m_flatPosX[index] - radius) / m_GridCellSize));
        const int maxX = static_cast<int>(floorf((m_flatPosX[index] + radius) / m_GridCellSize));
        const int minZ = static_cast<int>(floorf((m_flatPosZ[index] - radius) / m_GridCellSize));
        const int maxZ = static_cast<int>(floorf((m_flatPosZ[index] + radius) / m_GridCellSize));
        for (int cellZ = minZ; cellZ <= maxZ; ++cellZ) for (int cellX = minX; cellX <= maxX; ++cellX) {
            const uint64_t key = GridKey(cellX, cellZ);
            entries.emplace_back(key, index);
        }
    }

    std::sort(entries.begin(), entries.end());
    size_t begin = 0;
    while (begin < entries.size()) {
        size_t end = begin + 1;
        while (end < entries.size() && entries[end].first == entries[begin].first) ++end;
        for (size_t first = begin; first < end; ++first) for (size_t second = first + 1; second < end; ++second) {
            uint32_t a = entries[first].second, b = entries[second].second;
            testPair(a, b);
        }
        begin = end;
    }
}

void XPBDPhysicsSystem::RunGridGatherJob(void* rawContext) {
    const auto& context = *static_cast<GridGatherJobContext*>(rawContext);
    context.system->GatherDenseGridContacts(context.beginActiveCell, context.endActiveCell,
                                            context.width, context.height, context.minCellX,
                                            context.minCellZ, context.workerId);
}

void XPBDPhysicsSystem::RunGridStageJob(void* rawContext) {
    const auto& context = *static_cast<GridStageJobContext*>(rawContext);
    context.system->StageDenseGridContactPayloads(context.workerId, context.contactStart);
}

void XPBDPhysicsSystem::StageDenseGridContactPayloads(size_t workerId, size_t contactStart) {
    if (workerId >= m_GridContactCandidates.size()) return;
    const auto& candidates = m_GridContactCandidates[workerId];
    for (size_t offset = 0; offset < candidates.size(); ++offset) {
        const GridContactCandidate& candidate = candidates[offset];
        const uint32_t contactIndex = static_cast<uint32_t>(contactStart + offset);
        const size_t blockIndex = contactIndex / CONTACT_BLOCK_SIZE;
        const size_t laneIndex = contactIndex % CONTACT_BLOCK_SIZE;
        ContactBlock& block = m_ContactBlocks[blockIndex];
        const uint32_t flatIdxA = candidate.idxA;
        const uint32_t flatIdxB = candidate.idxB;
        const float inverseDistance = 1.0f / sqrtf(candidate.d2);
        const float normalX = candidate.dx * inverseDistance;
        const float normalZ = candidate.dz * inverseDistance;
        const float posAX = m_flatPosX[flatIdxA], posAZ = m_flatPosZ[flatIdxA];
        const float posBX = m_flatPosX[flatIdxB], posBZ = m_flatPosZ[flatIdxB];
        const float radiusA = m_flatRadius[flatIdxA];
        const float contactPointX = posAX + normalX * radiusA;
        const float contactPointZ = posAZ + normalZ * radiusA;
        block.idxA[laneIndex] = flatIdxA; block.idxB[laneIndex] = flatIdxB;
        block.posAX[laneIndex] = posAX; block.posAZ[laneIndex] = posAZ;
        block.posBX[laneIndex] = posBX; block.posBZ[laneIndex] = posBZ;
        block.radA[laneIndex] = radiusA; block.radB[laneIndex] = m_flatRadius[flatIdxB];
        block.invMassA[laneIndex] = m_flatInvMass[flatIdxA]; block.invMassB[laneIndex] = m_flatInvMass[flatIdxB];
        block.lambdaN[laneIndex] = 0.0f; block.lambdaT[laneIndex] = 0.0f;
        block.rotA[laneIndex] = m_flatRot[flatIdxA]; block.rotB[laneIndex] = m_flatRot[flatIdxB];
        block.invInertiaA[laneIndex] = m_flatInvInertia[flatIdxA]; block.invInertiaB[laneIndex] = m_flatInvInertia[flatIdxB];
        block.contactOffsetAX[laneIndex] = contactPointX - posAX; block.contactOffsetAZ[laneIndex] = contactPointZ - posAZ;
        block.contactOffsetBX[laneIndex] = contactPointX - posBX; block.contactOffsetBZ[laneIndex] = contactPointZ - posBZ;
        block.manifoldIndex[laneIndex] = UINT32_MAX;
        block.manifoldGeneration[laneIndex] = m_ManifoldGeneration;
    }
}

void XPBDPhysicsSystem::AssociateStagedDenseGridManifolds(size_t workerCount) {
    for (auto& contacts : m_DenseShardContacts) contacts.clear();
    for (size_t worker = 0; worker < workerCount; ++worker) {
        const auto& candidates = m_GridContactCandidates[worker];
        const size_t contactStart = m_GridStageContexts[worker].contactStart;
        for (size_t offset = 0; offset < candidates.size(); ++offset) {
            const GridContactCandidate& candidate = candidates[offset];
            const size_t shard = HashIndex(MakeKey(candidate.idxA, candidate.idxB)) & (DENSE_MANIFOLD_SHARDS - 1);
            m_DenseShardContacts[shard].push_back(static_cast<uint32_t>(contactStart + offset));
        }
    }
    for (size_t shard = 0; shard < DENSE_MANIFOLD_SHARDS; ++shard) {
        m_DenseShardStats[shard] = {};
        if (m_DenseShardCounts[shard] + m_DenseShardContacts[shard].size() > DENSE_MANIFOLD_SHARD_CAPACITY) {
            m_DenseShardCounts[shard] = 0;
            const size_t stampOffset = shard * DENSE_MANIFOLD_SHARD_STAMPS;
            std::fill(m_DenseShardStamp.begin() + stampOffset,
                      m_DenseShardStamp.begin() + stampOffset + DENSE_MANIFOLD_SHARD_STAMPS, UINT32_MAX);
        }
        m_DenseShardAssociateContexts[shard] = {this, shard};
        if (!m_DenseShardContacts[shard].empty())
            JobSystem::Get().ExecuteRaw(&XPBDPhysicsSystem::RunDenseShardAssociateJob, &m_DenseShardAssociateContexts[shard]);
    }
    JobSystem::Get().WaitForAll();
    if (m_ProbeMetricsEnabled) {
        for (const ManifoldCacheStats& stats : m_DenseShardStats) {
            m_ManifoldCacheStats.hits += stats.hits;
            m_ManifoldCacheStats.createAttempts += stats.createAttempts;
            m_ManifoldCacheStats.createSuccesses += stats.createSuccesses;
            m_ManifoldCacheStats.rejections += stats.rejections;
            m_ManifoldCacheStats.probeSteps += stats.probeSteps;
            m_ManifoldCacheStats.maxProbeDepth = std::max(m_ManifoldCacheStats.maxProbeDepth, stats.maxProbeDepth);
        }
    }
}

void XPBDPhysicsSystem::RunDenseShardAssociateJob(void* rawContext) {
    const auto& context = *static_cast<DenseShardAssociateContext*>(rawContext);
    context.system->AssociateDenseManifoldShard(context.shard);
}

void XPBDPhysicsSystem::AssociateDenseManifoldShard(size_t shard) {
    if (shard >= DENSE_MANIFOLD_SHARDS) return;
    ManifoldCacheStats& stats = m_DenseShardStats[shard];
    const bool collectMetrics = m_ProbeMetricsEnabled;
    const size_t stampOffset = shard * DENSE_MANIFOLD_SHARD_STAMPS;
    const size_t manifoldOffset = shard * DENSE_MANIFOLD_SHARD_CAPACITY;
    size_t shardManifoldCount = m_DenseShardCounts[shard];
    for (const uint32_t contactIndex : m_DenseShardContacts[shard]) {
        ContactBlock& block = m_ContactBlocks[contactIndex / CONTACT_BLOCK_SIZE];
        const size_t laneIndex = contactIndex % CONTACT_BLOCK_SIZE;
        const uint32_t a = block.idxA[laneIndex], b = block.idxB[laneIndex];
        const uint64_t key = MakeKey(a, b);
        size_t slot = (HashIndex(key) >> DENSE_MANIFOLD_SHARD_BITS) & (DENSE_MANIFOLD_SHARD_STAMPS - 1);
        int manifoldIndex = -1;
        bool cacheHit = false;
        bool lookupExhausted = true;
        size_t probeSteps = 0;
        for (size_t probe = 0; probe < DENSE_MANIFOLD_PROBE_LIMIT; ++probe) {
            ++probeSteps;
            const size_t stampIndex = stampOffset + slot;
            const uint32_t localIndex = m_DenseShardStamp[stampIndex];
            if (localIndex == UINT32_MAX) {
                if (collectMetrics) ++stats.createAttempts;
                if (shardManifoldCount >= DENSE_MANIFOLD_SHARD_CAPACITY) {
                    if (collectMetrics) ++stats.rejections;
                    break;
                }
                const size_t createdLocal = shardManifoldCount++;
                manifoldIndex = static_cast<int>(manifoldOffset + createdLocal);
                m_DenseShardStamp[stampIndex] = static_cast<uint32_t>(createdLocal);
                Manifold& manifold = m_DenseShardManifolds[manifoldIndex];
                manifold.idxA = a; manifold.idxB = b; manifold.lambdaN = 0.0f; manifold.lambdaT = 0.0f;
                if (collectMetrics) ++stats.createSuccesses;
                lookupExhausted = false;
                break;
            }
            const size_t candidateIndex = manifoldOffset + localIndex;
            const Manifold& manifold = m_DenseShardManifolds[candidateIndex];
            if (manifold.idxA == a && manifold.idxB == b) { manifoldIndex = static_cast<int>(candidateIndex); cacheHit = true; lookupExhausted = false; break; }
            slot = (slot + 1) & (DENSE_MANIFOLD_SHARD_STAMPS - 1);
        }
        if (manifoldIndex < 0 && lookupExhausted) {
            if (collectMetrics) { ++stats.createAttempts; ++stats.rejections; }
        }
        if (collectMetrics) {
            stats.probeSteps += probeSteps;
            stats.maxProbeDepth = std::max(stats.maxProbeDepth, probeSteps);
        }
        if (cacheHit) {
            if (collectMetrics) ++stats.hits;
            block.lambdaN[laneIndex] = m_DenseShardManifolds[manifoldIndex].lambdaN;
        }
        block.manifoldIndex[laneIndex] = manifoldIndex >= 0 ? DENSE_MANIFOLD_INDEX_FLAG | static_cast<uint32_t>(manifoldIndex) : UINT32_MAX;
        block.manifoldGeneration[laneIndex] = m_ManifoldGeneration;
    }
    m_DenseShardCounts[shard] = shardManifoldCount;
}

void XPBDPhysicsSystem::GatherDenseGridContacts(size_t beginActiveCell, size_t endActiveCell, int width, int height,
                                                 int minCellX, int minCellZ, size_t workerId) {
    if (workerId >= m_GridContactCandidates.size()) return;
    auto& output = m_GridContactCandidates[workerId];
    output.clear();
    m_GridCandidatePairCounts[workerId] = 0;
    const auto& heads = m_DenseGridHeads;
    const auto& next = m_DenseGridNext;
    const auto& activeCells = m_DenseGridActiveCells;
    size_t candidatePairCount = 0;
    const auto appendIfContact = [this, &output, &candidatePairCount](uint32_t a, uint32_t b) {
        if (!m_IsAwake[a] && !m_IsAwake[b]) return;
        if ((m_EntityLayers[a] & m_EntityLayers[b]) == 0) return;
        ++candidatePairCount;
        float dx = m_flatPosX[a] - m_flatPosX[b];
        float dz = m_flatPosZ[a] - m_flatPosZ[b];
        if (a > b) { std::swap(a, b); dx = -dx; dz = -dz; }
        const float radius = m_flatRadius[a] + m_flatRadius[b];
        if (fabsf(dx) > radius || fabsf(dz) > radius) return;
        const float distanceSquared = dx * dx + dz * dz;
        if (distanceSquared <= 0.0001f || distanceSquared >= radius * radius) return;
        const float minimumPenetrationRadius = radius - 0.0005f;
        if (minimumPenetrationRadius <= 0.0f ||
            distanceSquared > minimumPenetrationRadius * minimumPenetrationRadius) return;
        output.push_back({a, b, dx, dz, distanceSquared, radius, 0.0f});
    };
    for (size_t activeIndex = beginActiveCell; activeIndex < endActiveCell; ++activeIndex) {
        const size_t cell = m_DenseGridActiveCellList[activeIndex];
        const int x = static_cast<int>(cell % width);
        const int z = static_cast<int>(cell / width);
        const int cellHead = heads[cell];
        if (cellHead >= 0 && next[cellHead] >= 0)
            for (int first = cellHead; first >= 0; first = next[first])
                for (int second = next[first]; second >= 0; second = next[second])
                    appendIfContact(static_cast<uint32_t>(first), static_cast<uint32_t>(second));
        for (int offsetZ = -1; offsetZ <= 1; ++offsetZ) for (int offsetX = -1; offsetX <= 1; ++offsetX) {
            if (offsetX == 0 && offsetZ == 0) continue;
            const int neighborCellX = x + offsetX;
            const int neighborCellZ = z + offsetZ;
            if (neighborCellX < 0 || neighborCellX >= width || neighborCellZ < 0 || neighborCellZ >= height) continue;
            const size_t neighbor = static_cast<size_t>(neighborCellZ) * width + neighborCellX;
            if (activeCells[neighbor] && cell > neighbor) continue;
            const int neighborHead = heads[neighbor];
            if (neighborHead < 0) continue;
            for (int first = cellHead; first >= 0; first = next[first])
                for (int second = neighborHead; second >= 0; second = next[second])
                    appendIfContact(static_cast<uint32_t>(first), static_cast<uint32_t>(second));
        }
    }
    m_GridCandidatePairCounts[workerId] = candidatePairCount;
}

void XPBDPhysicsSystem::CCDPass(float dt) {
    if (m_BVHRoot < 0) return;
    for (size_t i = 0; i < m_activeFlatEntities; ++i) {
        if (!m_IsAwake[i]) continue;
        float vx = m_flatVelX[i], vz = m_flatVelZ[i];
        if (vx*vx + vz*vz < CCD_VELOCITY_THRESHOLD_SQ) continue;
        CCDQueryBVH(m_BVHRoot, (uint32_t)i, dt);
    }
}
void XPBDPhysicsSystem::CCDQueryBVH(int nodeIdx, uint32_t entityIdx, float dt) {
    if (nodeIdx < 0) return;
    m_BVHStack.clear(); m_BVHStack.push_back(nodeIdx);
    float px = m_flatPosX[entityIdx], pz = m_flatPosZ[entityIdx];
    float vx = m_flatVelX[entityIdx], vz = m_flatVelZ[entityIdx];
    float rA = m_flatRadius[entityIdx];
    while (!m_BVHStack.empty()) {
        int idx = m_BVHStack.back(); m_BVHStack.pop_back();
        if (idx < 0 || idx >= (int)m_BVHNodes.size()) continue;
        BVHNode& node = m_BVHNodes[idx];
        if (node.maxX < px - rA || node.minX > px + rA || node.maxZ < pz - rA || node.minZ > pz + rA) continue;
        if (node.isLeaf) {
            if (node.entityIdx < 0 || (uint32_t)node.entityIdx <= entityIdx) continue;
            if ((m_EntityLayers[entityIdx] & m_EntityLayers[node.entityIdx]) == 0) continue;
            uint32_t otherIdx = (uint32_t)node.entityIdx;
            float ox = m_flatPosX[otherIdx], oz = m_flatPosZ[otherIdx];
            float rB = m_flatRadius[otherIdx], sumR = rA + rB;
            float dx = px - ox, dz = pz - oz;
            float d2 = dx*dx + dz*dz;
            if (sqrtf(d2) <= sumR) continue;
            float ovx = m_flatVelX[otherIdx], ovz = m_flatVelZ[otherIdx];
            float relVX = vx - ovx, relVZ = vz - ovz;
            float a = relVX*relVX + relVZ*relVZ;
            if (a < 1e-12f) continue;
            float b = 2.0f * (dx*relVX + dz*relVZ);
            float c = d2 - sumR*sumR;
            float disc = b*b - 4.0f*a*c;
            if (disc < 0.0f) continue;
            float t = (-b - sqrtf(disc)) / (2.0f * a);
            if (t > 0.0f && t < dt) {
                float predAX = px + vx * t, predAZ = pz + vz * t;
                float predBX = ox + ovx * t, predBZ = oz + ovz * t;
                float pdx = predAX - predBX, pdz = predAZ - predBZ;
                float pd2 = pdx*pdx + pdz*pdz;
                EmitContact(entityIdx, otherIdx, pdx, pdz, pd2, sumR, 1.0f/sqrtf(pd2));
            }
        } else { m_BVHStack.push_back(node.left); m_BVHStack.push_back(node.right); }
    }
}

// Manifold & Contact
int XPBDPhysicsSystem::FindOrCreateManifold(uint32_t a, uint32_t b, bool& cacheHit) {
    cacheHit = false;
    if (a > b) std::swap(a, b);
    uint64_t key = MakeKey(a, b);
    size_t slot = HashIndex(key);
    size_t probeSteps = 0;
    for (int p = 0; p < 64; ++p) {
        ++probeSteps;
        uint32_t idx = m_ManifoldStamp[slot];
        if (idx == UINT32_MAX) {
            ++m_ManifoldCacheStats.createAttempts;
            if (m_ManifoldCount >= MAX_MANIFOLDS) {
                EvictManifolds();
                if (m_ManifoldCount >= MAX_MANIFOLDS) {
                    ++m_ManifoldCacheStats.rejections;
                    return -1;
                }
                slot = HashIndex(key);
                p = -1;
                continue;
            }
            const int created = m_ManifoldCount++;
            m_ManifoldStamp[slot] = static_cast<uint32_t>(created);
            ++m_ManifoldCacheStats.createSuccesses;
            m_Manifolds[created].idxA = a; m_Manifolds[created].idxB = b;
            m_Manifolds[created].lambdaN = 0.0f;
            m_Manifolds[created].lambdaT = 0.0f;
            if (m_ProbeMetricsEnabled) {
                m_ManifoldCacheStats.probeSteps += probeSteps;
                m_ManifoldCacheStats.maxProbeDepth = std::max(m_ManifoldCacheStats.maxProbeDepth, probeSteps);
            }
            return created;
        }
        if (m_Manifolds[idx].idxA == a && m_Manifolds[idx].idxB == b) {
            cacheHit = true;
            if (m_ProbeMetricsEnabled) {
                m_ManifoldCacheStats.probeSteps += probeSteps;
                m_ManifoldCacheStats.maxProbeDepth = std::max(m_ManifoldCacheStats.maxProbeDepth, probeSteps);
            }
            return idx;
        }
        slot = (slot + 1) & (STAMP_SIZE - 1);
    }
    ++m_ManifoldCacheStats.createAttempts;
    ++m_ManifoldCacheStats.rejections;
    if (m_ProbeMetricsEnabled) {
        m_ManifoldCacheStats.probeSteps += probeSteps;
        m_ManifoldCacheStats.maxProbeDepth = std::max(m_ManifoldCacheStats.maxProbeDepth, probeSteps);
    }
    return -1;
}
void XPBDPhysicsSystem::EvictManifolds() {
    if (m_ManifoldCount < MAX_MANIFOLDS * 0.9f) return;
    m_ManifoldCount = 0;
    std::fill(m_ManifoldStamp.begin(), m_ManifoldStamp.end(), UINT32_MAX);
    ++m_ManifoldGeneration;
    if (m_ManifoldGeneration == 0) m_ManifoldGeneration = 1;
}

bool XPBDPhysicsSystem::PairStampExists(uint64_t key) {
    size_t slot = HashIndex(key);
    for (uint32_t p = 0; p < PAIR_PROBE_LIMIT; ++p) {
        if (m_PairStamp[slot] != m_CurrentStamp) { m_PairKeys[slot] = key; m_PairStamp[slot] = m_CurrentStamp; return false; }
        if (m_PairKeys[slot] == key) return true;
        slot = (slot + 1) & (STAMP_SIZE - 1);
    }
    m_PairKeys[slot] = key; m_PairStamp[slot] = m_CurrentStamp;
    return false;
}

void XPBDPhysicsSystem::EmitContact(uint32_t flatIdxA, uint32_t flatIdxB,
    float dx, float dz, float d2, float sumR, float invDist, bool pairAlreadyUnique, bool indicesCanonical) {
    if (!indicesCanonical) {
        if (flatIdxA >= m_activeFlatEntities || flatIdxB >= m_activeFlatEntities) return;
        if (flatIdxA > flatIdxB) std::swap(flatIdxA, flatIdxB);
    }
    if (!pairAlreadyUnique) {
        uint64_t key = MakeKey(flatIdxA, flatIdxB);
        if (PairStampExists(key)) return;
    }
    float dist = d2 * invDist; float pene = sumR - dist;
    if (pene < 0.0005f) return;
    float nx = dx * invDist, nz = dz * invDist;
    float cpX = m_flatPosX[flatIdxA] + nx * m_flatRadius[flatIdxA];
    float cpZ = m_flatPosZ[flatIdxA] + nz * m_flatRadius[flatIdxA];
    float offAX = cpX - m_flatPosX[flatIdxA], offAZ = cpZ - m_flatPosZ[flatIdxA];
    float offBX = cpX - m_flatPosX[flatIdxB], offBZ = cpZ - m_flatPosZ[flatIdxB];
    bool cacheHit = false;
    int manIdx = FindOrCreateManifold(flatIdxA, flatIdxB, cacheHit);
    float lamN = 0.0f;
    if (cacheHit) {
        ++m_ManifoldCacheStats.hits;
        lamN = m_Manifolds[manIdx].lambdaN;
    }
    if (m_ContactCount >= CONTACT_BUDGET) return;
    size_t blockIdx = m_ContactCount / CONTACT_BLOCK_SIZE;
    size_t laneIdx = m_ContactCount % CONTACT_BLOCK_SIZE;
    ContactBlock& blk = m_ContactBlocks[blockIdx];
    blk.idxA[laneIdx] = flatIdxA; blk.idxB[laneIdx] = flatIdxB;
    blk.posAX[laneIdx] = m_flatPosX[flatIdxA]; blk.posAZ[laneIdx] = m_flatPosZ[flatIdxA];
    blk.posBX[laneIdx] = m_flatPosX[flatIdxB]; blk.posBZ[laneIdx] = m_flatPosZ[flatIdxB];
    blk.radA[laneIdx] = m_flatRadius[flatIdxA]; blk.radB[laneIdx] = m_flatRadius[flatIdxB];
    blk.invMassA[laneIdx] = m_flatInvMass[flatIdxA]; blk.invMassB[laneIdx] = m_flatInvMass[flatIdxB];
    blk.lambdaN[laneIdx] = lamN; blk.lambdaT[laneIdx] = 0;
    blk.rotA[laneIdx] = m_flatRot[flatIdxA]; blk.rotB[laneIdx] = m_flatRot[flatIdxB];
    blk.invInertiaA[laneIdx] = m_flatInvInertia[flatIdxA]; blk.invInertiaB[laneIdx] = m_flatInvInertia[flatIdxB];
    blk.contactOffsetAX[laneIdx] = offAX; blk.contactOffsetAZ[laneIdx] = offAZ;
    blk.contactOffsetBX[laneIdx] = offBX; blk.contactOffsetBZ[laneIdx] = offBZ;
    blk.manifoldIndex[laneIdx] = manIdx >= 0 ? static_cast<uint32_t>(manIdx) : UINT32_MAX;
    blk.manifoldGeneration[laneIdx] = m_ManifoldGeneration;
    m_ContactCount++;
}

// Islands & Graph
void XPBDPhysicsSystem::BuildLocalIslands() {
    const size_t totalEntities = m_activeFlatEntities;
    m_IslandRanges.clear();
    if (m_ContactCount == 0) return;
    auto find = [this](uint32_t x)->uint32_t {
        while (m_UF_Parent[x] != x) { m_UF_Parent[x] = m_UF_Parent[m_UF_Parent[x]]; x = m_UF_Parent[x]; }
        return x;
    };
    m_RootToIslandIndex.assign(totalEntities, UINT32_MAX);
    m_IslandRoots.clear();
    for (size_t ci = 0; ci < m_ContactCount; ++ci) {
        const size_t bi = ci / CONTACT_BLOCK_SIZE;
        const size_t li = ci % CONTACT_BLOCK_SIZE;
        const uint32_t a = m_ContactBlocks[bi].idxA[li];
        const uint32_t b = m_ContactBlocks[bi].idxB[li];
        if (a >= totalEntities || b >= totalEntities) continue;
        const uint32_t r = find(a);
        m_UF_Parent[a] = r;
        m_UF_Parent[b] = r;
        if (m_RootToIslandIndex[r] == UINT32_MAX) {
            m_RootToIslandIndex[r] = (uint32_t)m_IslandRoots.size();
            m_IslandRoots.push_back(r);
        }
    }
    m_NumIslands = m_IslandRoots.size();
    m_IslandSizes.assign(m_NumIslands, 0);
    m_IslandContactBlock.clear();
    for (size_t ci = 0; ci < m_ContactCount; ++ci) {
        size_t bi = ci / CONTACT_BLOCK_SIZE, li = ci % CONTACT_BLOCK_SIZE;
        uint32_t r = find(m_ContactBlocks[bi].idxA[li]);
        uint32_t isl = m_RootToIslandIndex[r];
        if (isl == UINT32_MAX) continue;
        m_IslandSizes[isl]++;
    }
    m_IslandRanges.resize(m_NumIslands);
    uint32_t off = 0;
    for (size_t isl = 0; isl < m_NumIslands; ++isl) {
        m_IslandRanges[isl].start = off;
        m_IslandRanges[isl].count = m_IslandSizes[isl];
        off += m_IslandSizes[isl];
        m_IslandSizes[isl] = 0;
    }
    m_IslandContactBlock.resize(m_ContactCount);
    for (size_t ci = 0; ci < m_ContactCount; ++ci) {
        size_t bi = ci / CONTACT_BLOCK_SIZE, li = ci % CONTACT_BLOCK_SIZE;
        uint32_t r = find(m_ContactBlocks[bi].idxA[li]);
        uint32_t isl = m_RootToIslandIndex[r];
        if (isl == UINT32_MAX) continue;
        uint32_t pos = m_IslandRanges[isl].start + m_IslandSizes[isl];
        m_IslandContactBlock[pos] = (uint32_t)ci;
        m_IslandSizes[isl]++;
    }
    m_IslandDirty.assign(m_NumIslands, 0);
    if (m_IslandRootsPrev.size() == m_IslandRoots.size()) {
        for (size_t isl = 0; isl < m_NumIslands; ++isl)
            if (m_IslandRoots[isl] != m_IslandRootsPrev[isl] || m_IslandSizesPrev[isl] != m_IslandRanges[isl].count)
                m_IslandDirty[isl] = 1;
    } else m_IslandDirty.assign(m_NumIslands, 1);
    m_IslandRootsPrev = m_IslandRoots;
    m_IslandSizesPrev.assign(m_NumIslands, 0);
    for (size_t isl = 0; isl < m_NumIslands; ++isl) m_IslandSizesPrev[isl] = m_IslandRanges[isl].count;
}
void XPBDPhysicsSystem::BuildConstraintGraph() {
    m_ColorBatches.clear();
    m_SortedContactIndices.resize(m_ContactCount);
    m_ContactColors.assign(m_ContactCount, 0);
    if (m_EntityColorMasks.size() < m_activeFlatEntities) m_EntityColorMasks.resize(m_activeFlatEntities, 0);
    std::fill(m_EntityColorMasks.begin(), m_EntityColorMasks.begin() + m_activeFlatEntities, 0);
    uint32_t colorStarts[MAX_GRAPH_COLORS] = {0};
    uint32_t colorCounts[MAX_GRAPH_COLORS] = {0};
    for (uint32_t ci = 0; ci < m_ContactCount; ++ci) {
        size_t bi = ci / CONTACT_BLOCK_SIZE, li = ci % CONTACT_BLOCK_SIZE;
        uint32_t idxA = m_ContactBlocks[bi].idxA[li];
        uint32_t idxB = m_ContactBlocks[bi].idxB[li];
        const uint64_t used = m_EntityColorMasks[idxA] | m_EntityColorMasks[idxB];
        const uint64_t freeColors = ~used;
        int8_t chosenColor = freeColors == 0 ? 0 : static_cast<int8_t>(std::countr_zero(freeColors));
        if (chosenColor >= MAX_GRAPH_COLORS) chosenColor = 0;
        m_EntityColorMasks[idxA] |= (uint64_t{1} << chosenColor);
        m_EntityColorMasks[idxB] |= (uint64_t{1} << chosenColor);
        m_ContactColors[ci] = static_cast<uint8_t>(chosenColor);
        colorCounts[chosenColor]++;
    }
    uint32_t offset = 0;
    m_NumColors = 0;
    for (int c = 0; c < MAX_GRAPH_COLORS; ++c) {
        if (colorCounts[c] > 0) { colorStarts[c] = offset; offset += colorCounts[c]; m_NumColors = c + 1; }
    }
    uint32_t placeIdx[MAX_GRAPH_COLORS];
    memcpy(placeIdx, colorStarts, sizeof(colorStarts));
    for (uint32_t ci = 0; ci < m_ContactCount; ++ci) {
        const uint8_t col = m_ContactColors[ci];
        m_SortedContactIndices[placeIdx[col]++] = ci;
    }
    for (int c = 0; c < m_NumColors; ++c)
        if (colorCounts[c] > 0) m_ColorBatches.push_back({colorStarts[c], colorCounts[c]});
}

// Solver
void XPBDPhysicsSystem::SolveConstraintsColored(float dt, size_t threadId) {
    if (threadId >= MAX_WORKER_THREADS) return;
    if (m_ConstraintCount == 0) return;
    auto& ld = m_ThreadDeltas[threadId];
    const size_t numThreads = std::max(JobSystem::Get().NumWorkers(), 1UL);
    size_t chunkSize = (m_ConstraintCount + numThreads - 1) / numThreads;
    size_t start = threadId * chunkSize;
    size_t end = std::min(start + chunkSize, m_ConstraintCount);
    if (start < end) m_ThreadDeltaActive[threadId] = 1;
    for (size_t i = start; i < end; ++i) {
        if (i >= m_ConstraintCount) break;
        Constraint& cn = m_Constraints[i];
        if (!cn.active) continue;
        uint32_t a = cn.idxA, b = cn.idxB;
        if (a >= m_activeFlatEntities || b >= m_activeFlatEntities) { cn.active = false; continue; }
        float dx = (m_flatPosX[a] + cn.anchorAX) - (m_flatPosX[b] + cn.anchorBX);
        float dz = (m_flatPosZ[a] + cn.anchorAZ) - (m_flatPosZ[b] + cn.anchorBZ);
        float invMassA = m_flatInvMass[a], invMassB = m_flatInvMass[b];
        float w = invMassA + invMassB;
        if (w < 1e-12f) continue;
        float compliance = 1.0f / (std::max(cn.stiffness, 1.0f) * dt * dt);
        if (cn.type == ConstraintType::Hinge || cn.type == ConstraintType::MotorizedHinge ||
            cn.type == ConstraintType::Fixed || cn.type == ConstraintType::Spherical ||
            cn.type == ConstraintType::Distance) {
            const float distance = sqrtf(dx*dx + dz*dz);
            if (distance < 1e-8f) continue;
            float C = distance;
            if (cn.type == ConstraintType::Distance) {
                if (distance < cn.limitLow) C = distance - cn.limitLow;
                else if (distance > cn.limitHigh) C = distance - cn.limitHigh;
                else continue;
            }
            float nx = dx/distance, nz = dz/distance;
            float denom = w + compliance;
            float dlambda = -(C + compliance * cn.lambda[0]) / denom;
            if (cn.maxForce > 0.0f) {
                const float maxImpulse = cn.maxForce * dt;
                dlambda = std::clamp(dlambda, -maxImpulse, maxImpulse);
            }
            cn.lambda[0] += dlambda;
            if (cn.breakThreshold > 0.0f && fabsf(cn.lambda[0]) > cn.breakThreshold) { cn.active = false; continue; }
            float impulse = dlambda;
            ld[a].posX += impulse * nx * invMassA; ld[a].posZ += impulse * nz * invMassA;
            ld[b].posX -= impulse * nx * invMassB; ld[b].posZ -= impulse * nz * invMassB;
            if (cn.type == ConstraintType::Fixed || cn.type == ConstraintType::MotorizedHinge ||
                cn.axisX != 0.0f || cn.axisZ != 0.0f) {
                float angDiff = m_flatRot[a] - m_flatRot[b] - cn.motorSpeed * dt;
                float invI = m_flatInvInertia[a] + m_flatInvInertia[b];
                if (invI > 1e-12f) {
                    float dlamRot = -(angDiff + compliance * cn.lambda[1]) / (invI + compliance);
                    cn.lambda[1] += dlamRot;
                    ld[a].rot += dlamRot * m_flatInvInertia[a];
                    ld[b].rot -= dlamRot * m_flatInvInertia[b];
                }
            }
        } else if (cn.type == ConstraintType::ConeTwist) {
            float C = sqrtf(dx*dx + dz*dz);
            float nx = (C > 1e-8f) ? dx/C : 0.0f, nz = (C > 1e-8f) ? dz/C : 0.0f;
            float denom = w + compliance;
            float dlambda = -(C + compliance * cn.lambda[0]) / denom;
            cn.lambda[0] += dlambda;
            float impulse = dlambda;
            ld[a].posX += impulse * nx * invMassA; ld[a].posZ += impulse * nz * invMassA;
            ld[b].posX -= impulse * nx * invMassB; ld[b].posZ -= impulse * nz * invMassB;
            float angDiff = m_flatRot[a] - m_flatRot[b];
            float invI = m_flatInvInertia[a] + m_flatInvInertia[b];
            if (invI > 1e-12f) {
                float dlamRot = -(angDiff + compliance * cn.lambda[1]) / (invI + compliance);
                cn.lambda[1] += dlamRot;
                ld[a].rot += dlamRot * m_flatInvInertia[a];
                ld[b].rot -= dlamRot * m_flatInvInertia[b];
            }
        } else if (cn.type == ConstraintType::Prismatic) {
            float proj = dx * cn.axisX + dz * cn.axisZ;
            float C = proj;
            if (cn.limitLow != 0.0f || cn.limitHigh != 0.0f) {
                if (C < cn.limitLow) C -= cn.limitLow;
                else if (C > cn.limitHigh) C -= cn.limitHigh;
                else continue;
            }
            float denom = w + compliance;
            float dlambda = -(C + compliance * cn.lambda[0]) / denom;
            cn.lambda[0] += dlambda;
            float impulse = dlambda;
            ld[a].posX += impulse * cn.axisX * invMassA; ld[a].posZ += impulse * cn.axisZ * invMassA;
            ld[b].posX -= impulse * cn.axisX * invMassB; ld[b].posZ -= impulse * cn.axisZ * invMassB;
        }
    }
}
void XPBDPhysicsSystem::SolveCloth(float dt, size_t threadId) {
    if (m_ClothPatches.empty() || threadId >= MAX_WORKER_THREADS) return;
    m_ThreadDeltaActive[threadId] = 1;
    auto& ld = m_ThreadDeltas[threadId];
    for (auto& patch : m_ClothPatches) {
        if (patch.particles.size() < 2) continue;
        float compliance = 0.0001f / (dt * dt);
        for (size_t i = 0; i < patch.particles.size()-1; ++i) {
            uint32_t a = patch.particles[i].entityIdx, b = patch.particles[i+1].entityIdx;
            if (a >= m_activeFlatEntities || b >= m_activeFlatEntities) continue;
            float dx = m_flatPosX[a] - m_flatPosX[b], dz = m_flatPosZ[a] - m_flatPosZ[b];
            float rest = sqrtf((patch.particles[i].uvX-patch.particles[i+1].uvX)*(patch.particles[i].uvX-patch.particles[i+1].uvX) +
                               (patch.particles[i].uvZ-patch.particles[i+1].uvZ)*(patch.particles[i].uvZ-patch.particles[i+1].uvZ));
            float dist = sqrtf(dx*dx+dz*dz);
            if (dist < 1e-8f) continue;
            float C = dist - rest;
            float invMassA = patch.particles[i].invMass, invMassB = patch.particles[i+1].invMass;
            float w = invMassA + invMassB;
            if (w < 1e-12f) continue;
            float denom = w + compliance;
            float dlambda = -(C + compliance * 0.0f) / denom;
            float impulse = dlambda * patch.stiffness;
            float nx = dx/dist, nz = dz/dist;
            ld[a].posX += impulse * nx * invMassA; ld[a].posZ += impulse * nz * invMassA;
            ld[b].posX -= impulse * nx * invMassB; ld[b].posZ -= impulse * nz * invMassB;
        }
    }
}
void XPBDPhysicsSystem::SolveColorBatch(size_t colorIdx, float compliance, float dt, size_t threadId, uint32_t iterationWeight,
                                        size_t contactOffset, size_t contactCount) {
    if (threadId >= MAX_WORKER_THREADS) return;
    if (colorIdx >= m_ColorBatches.size()) return;
    const ColorBatch& batch = m_ColorBatches[colorIdx];
    if (contactOffset >= batch.count) return;
    const size_t count = std::min(contactCount, static_cast<size_t>(batch.count) - contactOffset);
    if (count == 0) return;
    m_ThreadDeltaActive[threadId] = 1;
    size_t numLanes = (count + 3) / 4;
    std::vector<SolveLane>& solveBuffer = m_SolveBuffers[threadId];
    if (solveBuffer.size() < numLanes) solveBuffer.resize(numLanes);
    std::vector<DeferredDelta>& localDelta = m_ThreadDeltas[threadId];
    const size_t limit = m_activeFlatEntities;
    const float iterationScale = static_cast<float>(iterationWeight);

    for (size_t li = 0; li < numLanes; ++li) {
        SolveLane& buf = solveBuffer[li];
        for (int k = 0; k < 4; ++k) { buf.idxA[k] = UINT32_MAX; buf.idxB[k] = UINT32_MAX; }
    }
    for (size_t i = 0; i < count; ++i) {
        uint32_t ci = m_SortedContactIndices[batch.start + contactOffset + i];
        if (ci >= m_ContactCount) continue;
        size_t bi = ci / CONTACT_BLOCK_SIZE, li = ci % CONTACT_BLOCK_SIZE;
        if (bi >= m_ContactBlocks.size()) continue;
        ContactBlock& blk = m_ContactBlocks[bi];
        size_t laneIdx = i / 4, lane = i % 4;
        SolveLane& buf = solveBuffer[laneIdx];
        buf.posAX[lane] = blk.posAX[li]; buf.posAZ[lane] = blk.posAZ[li];
        buf.posBX[lane] = blk.posBX[li]; buf.posBZ[lane] = blk.posBZ[li];
        buf.radA[lane] = blk.radA[li]; buf.radB[lane] = blk.radB[li];
        buf.invMassA[lane] = blk.invMassA[li]; buf.invMassB[lane] = blk.invMassB[li];
        buf.lambdaN[lane] = blk.lambdaN[li]; buf.lambdaT[lane] = blk.lambdaT[li];
        buf.idxA[lane] = blk.idxA[li]; buf.idxB[lane] = blk.idxB[li];
        buf.rotA[lane] = blk.rotA[li]; buf.rotB[lane] = blk.rotB[li];
        buf.invInertiaA[lane] = blk.invInertiaA[li]; buf.invInertiaB[lane] = blk.invInertiaB[li];
        buf.rAX[lane] = blk.contactOffsetAX[li]; buf.rAZ[lane] = blk.contactOffsetAZ[li];
        buf.rBX[lane] = blk.contactOffsetBX[li]; buf.rBZ[lane] = blk.contactOffsetBZ[li];
        buf.manifoldIndex[lane] = blk.manifoldIndex[li];
        buf.manifoldGeneration[lane] = blk.manifoldGeneration[li];
    }

    float32x4_t comp4 = vdupq_n_f32(compliance);
    float32x4_t friction4 = vdupq_n_f32(FRICTION_COEFF);

    for (size_t li = 0; li < numLanes; ++li) {
        SolveLane& buf = solveBuffer[li];
        float32x4_t pAX = vld1q_f32(buf.posAX), pAZ = vld1q_f32(buf.posAZ);
        float32x4_t pBX = vld1q_f32(buf.posBX), pBZ = vld1q_f32(buf.posBZ);
        float32x4_t imA = vld1q_f32(buf.invMassA), imB = vld1q_f32(buf.invMassB);
        float32x4_t iiA = vld1q_f32(buf.invInertiaA), iiB = vld1q_f32(buf.invInertiaB);
        float32x4_t rAX = vld1q_f32(buf.rAX), rAZ = vld1q_f32(buf.rAZ);
        float32x4_t rBX = vld1q_f32(buf.rBX), rBZ = vld1q_f32(buf.rBZ);
        float32x4_t lambdaN = vld1q_f32(buf.lambdaN);

        float32x4_t dx4 = vsubq_f32(pAX, pBX), dz4 = vsubq_f32(pAZ, pBZ);
        float32x4_t d24 = vaddq_f32(vmulq_f32(dx4,dx4), vmulq_f32(dz4,dz4));
        float32x4_t radA = vld1q_f32(buf.radA), radB = vld1q_f32(buf.radB);
        float32x4_t sumR4 = vaddq_f32(radA, radB), sumRSq4 = vmulq_f32(sumR4, sumR4);
        float32x4_t invD4 = FastRsqrt4(d24);
        float32x4_t dist4 = vmulq_f32(d24, invD4), C4 = vsubq_f32(dist4, sumR4);
        uint32x4_t valid = vandq_u32(vcltq_f32(d24, sumRSq4), vcgtq_f32(d24, vdupq_n_f32(1e-12f)));
        valid = vandq_u32(valid, vcltq_f32(C4, vdupq_n_f32(0.0001f)));
        if (!simdAnyMask(valid)) continue;

        float32x4_t nx4 = vmulq_f32(dx4, invD4), nz4 = vmulq_f32(dz4, invD4);
        float32x4_t ranA = vsubq_f32(vmulq_f32(rAX, nz4), vmulq_f32(rAZ, nx4));
        float32x4_t ranB = vsubq_f32(vmulq_f32(rBX, nz4), vmulq_f32(rBZ, nx4));
        float32x4_t rotMassA = vmulq_f32(vmulq_f32(ranA, ranA), iiA);
        float32x4_t rotMassB = vmulq_f32(vmulq_f32(ranB, ranB), iiB);
        float32x4_t w4 = vaddq_f32(vaddq_f32(imA, imB), vaddq_f32(rotMassA, rotMassB));
        float32x4_t den = vaddq_f32(w4, comp4);
        float32x4_t invDen = FastReciprocal4(den);
        float32x4_t num = vnegq_f32(vaddq_f32(C4, vmulq_f32(comp4, lambdaN)));
        float32x4_t dL4 = vmulq_f32(num, invDen);
        float32x4_t newLN = vaddq_f32(lambdaN, dL4);
        uint32x4_t negM = vcltq_f32(newLN, vdupq_n_f32(0.0f));
        dL4 = vbslq_f32(negM, vsubq_f32(vdupq_n_f32(0.0f), lambdaN), dL4);
        newLN = vmaxq_f32(newLN, vdupq_n_f32(0.0f));

        float32x4_t px4 = vmulq_f32(nx4, dL4), pz4 = vmulq_f32(nz4, dL4);
        float32x4_t dAX = vmulq_f32(px4, imA), dAZ = vmulq_f32(pz4, imA);
        float32x4_t dBX = vmulq_f32(px4, imB), dBZ = vmulq_f32(pz4, imB);
        float32x4_t newAX = vaddq_f32(pAX, dAX), newAZ = vaddq_f32(pAZ, dAZ);
        float32x4_t newBX = vsubq_f32(pBX, dBX), newBZ = vsubq_f32(pBZ, dBZ);
        float32x4_t dRotA = vmulq_f32(vmulq_f32(ranA, dL4), iiA);
        float32x4_t dRotB = vmulq_f32(vmulq_f32(ranB, dL4), iiB);

        float32x4_t tx4 = vnegq_f32(nz4), tz4 = nx4;
        float32x4_t oldAX = pAX, oldAZ = pAZ, oldBX = pBX, oldBZ = pBZ;
        float32x4_t deltaAX = vsubq_f32(newAX, oldAX), deltaAZ = vsubq_f32(newAZ, oldAZ);
        float32x4_t deltaBX = vsubq_f32(newBX, oldBX), deltaBZ = vsubq_f32(newBZ, oldBZ);
        float32x4_t relDX = vsubq_f32(deltaAX, deltaBX), relDZ = vsubq_f32(deltaAZ, deltaBZ);
        float32x4_t relT = vaddq_f32(vmulq_f32(relDX, tx4), vmulq_f32(relDZ, tz4));
        float32x4_t wFric = vaddq_f32(imA, imB);
        float32x4_t invWFric = FastReciprocal4(wFric);
        float32x4_t dLT = vmulq_f32(vnegq_f32(relT), invWFric);
        float32x4_t lambdaT = vld1q_f32(buf.lambdaT);
        float32x4_t newLT = vaddq_f32(lambdaT, dLT);
        float32x4_t maxFric = vmulq_f32(newLN, friction4);
        newLT = vmaxq_f32(newLT, vnegq_f32(maxFric));
        newLT = vminq_f32(newLT, maxFric);
        dLT = vsubq_f32(newLT, lambdaT);
        float32x4_t fricAX = vmulq_f32(dLT, vmulq_f32(tx4, imA));
        float32x4_t fricAZ = vmulq_f32(dLT, vmulq_f32(tz4, imA));
        float32x4_t fricBX = vmulq_f32(dLT, vmulq_f32(tx4, imB));
        float32x4_t fricBZ = vmulq_f32(dLT, vmulq_f32(tz4, imB));
        newAX = vaddq_f32(newAX, fricAX); newAZ = vaddq_f32(newAZ, fricAZ);
        newBX = vsubq_f32(newBX, fricBX); newBZ = vsubq_f32(newBZ, fricBZ);

        vst1q_f32(buf.posAX, newAX); vst1q_f32(buf.posAZ, newAZ);
        vst1q_f32(buf.posBX, newBX); vst1q_f32(buf.posBZ, newBZ);
        vst1q_f32(buf.lambdaN, newLN); vst1q_f32(buf.lambdaT, newLT);

        uint32_t idxA[4], idxB[4];
        vst1q_u32(idxA, vld1q_u32(buf.idxA)); vst1q_u32(idxB, vld1q_u32(buf.idxB));
        uint32_t val[4]; vst1q_u32(val, valid);
        for (int k = 0; k < 4; ++k) {
            if (!val[k] || idxA[k] == UINT32_MAX || idxB[k] == UINT32_MAX) continue;
            if (idxA[k] >= limit || idxB[k] >= limit) continue;
            localDelta[idxA[k]].posX += (newAX[k] - oldAX[k]) * iterationScale;
            localDelta[idxA[k]].posZ += (newAZ[k] - oldAZ[k]) * iterationScale;
            localDelta[idxB[k]].posX += (newBX[k] - oldBX[k]) * iterationScale;
            localDelta[idxB[k]].posZ += (newBZ[k] - oldBZ[k]) * iterationScale;
            localDelta[idxA[k]].rot += dRotA[k] * iterationScale;
            localDelta[idxB[k]].rot -= dRotB[k] * iterationScale;
            const uint32_t manifoldIndex = buf.manifoldIndex[k];
            if (buf.manifoldGeneration[k] == m_ManifoldGeneration) {
                if ((manifoldIndex & DENSE_MANIFOLD_INDEX_FLAG) != 0) {
                    const uint32_t denseIndex = manifoldIndex & ~DENSE_MANIFOLD_INDEX_FLAG;
                    if (denseIndex < m_DenseShardManifolds.size()) {
                        m_DenseShardManifolds[denseIndex].lambdaN = newLN[k];
                        m_DenseShardManifolds[denseIndex].lambdaT = newLT[k];
                    }
                } else if (manifoldIndex < static_cast<uint32_t>(m_ManifoldCount)) {
                    m_Manifolds[manifoldIndex].lambdaN = newLN[k];
                    m_Manifolds[manifoldIndex].lambdaT = newLT[k];
                }
            }
        }
    }

    for (size_t li = 0; li < numLanes; ++li) {
        SolveLane& buf = solveBuffer[li];
        uint32_t idxA[4], idxB[4];
        vst1q_u32(idxA, vld1q_u32(buf.idxA)); vst1q_u32(idxB, vld1q_u32(buf.idxB));
        float32x4_t imA = vld1q_f32(buf.invMassA), imB = vld1q_f32(buf.invMassB);
        float32x4_t iiA = vld1q_f32(buf.invInertiaA), iiB = vld1q_f32(buf.invInertiaB);
        float32x4_t dx4 = vsubq_f32(vld1q_f32(buf.posAX), vld1q_f32(buf.posBX));
        float32x4_t dz4 = vsubq_f32(vld1q_f32(buf.posAZ), vld1q_f32(buf.posBZ));
        float32x4_t invDist4 = FastRsqrt4(vaddq_f32(vmulq_f32(dx4,dx4), vmulq_f32(dz4,dz4)));
        float32x4_t nx4 = vmulq_f32(dx4, invDist4), nz4 = vmulq_f32(dz4, invDist4);

        float vAx[4]={0,0,0,0}, vAz[4]={0,0,0,0}, vBx[4]={0,0,0,0}, vBz[4]={0,0,0,0};
        float aA[4]={0,0,0,0}, aB[4]={0,0,0,0};
        for (int k=0;k<4;++k) {
            if (idxA[k]==UINT32_MAX || idxB[k]==UINT32_MAX) continue;
            if (idxA[k]>=limit || idxB[k]>=limit) continue;
            vAx[k]=m_flatVelX[idxA[k]]; vAz[k]=m_flatVelZ[idxA[k]];
            vBx[k]=m_flatVelX[idxB[k]]; vBz[k]=m_flatVelZ[idxB[k]];
            aA[k]=m_flatAngVel[idxA[k]]; aB[k]=m_flatAngVel[idxB[k]];
        }
        float32x4_t velAX=vld1q_f32(vAx), velAZ=vld1q_f32(vAz);
        float32x4_t velBX=vld1q_f32(vBx), velBZ=vld1q_f32(vBz);
        float32x4_t angA=vld1q_f32(aA), angB=vld1q_f32(aB);
        float32x4_t rAX=vld1q_f32(buf.rAX), rAZ=vld1q_f32(buf.rAZ);
        float32x4_t rBX=vld1q_f32(buf.rBX), rBZ=vld1q_f32(buf.rBZ);

        float32x4_t tanVelAX=vmulq_f32(vnegq_f32(angA), rAZ);
        float32x4_t tanVelAZ=vmulq_f32(angA, rAX);
        float32x4_t tanVelBX=vmulq_f32(vnegq_f32(angB), rBZ);
        float32x4_t tanVelBZ=vmulq_f32(angB, rBX);
        float32x4_t relVX=vsubq_f32(vaddq_f32(velAX, tanVelAX), vaddq_f32(velBX, tanVelBX));
        float32x4_t relVZ=vsubq_f32(vaddq_f32(velAZ, tanVelAZ), vaddq_f32(velBZ, tanVelBZ));
        float32x4_t relVN=vaddq_f32(vmulq_f32(relVX, nx4), vmulq_f32(relVZ, nz4));
        float32x4_t ranA=vsubq_f32(vmulq_f32(rAX, nz4), vmulq_f32(rAZ, nx4));
        float32x4_t ranB=vsubq_f32(vmulq_f32(rBX, nz4), vmulq_f32(rBZ, nx4));
        float32x4_t rotMassA=vmulq_f32(vmulq_f32(ranA, ranA), iiA);
        float32x4_t rotMassB=vmulq_f32(vmulq_f32(ranB, ranB), iiB);
        float32x4_t w4=vaddq_f32(vaddq_f32(imA, imB), vaddq_f32(rotMassA, rotMassB));
        float32x4_t invW4=FastReciprocal4(w4);
        float32x4_t j4=vmulq_f32(vnegq_f32(vmulq_f32(vdupq_n_f32(1.0f+RESTITUTION), relVN)), invW4);
        uint32x4_t approach=vcltq_f32(relVN, vdupq_n_f32(0.0f));
        j4=vbslq_f32(approach, j4, vdupq_n_f32(0.0f));

        float32x4_t dVAX=vmulq_f32(nx4, vmulq_f32(j4, imA));
        float32x4_t dVAZ=vmulq_f32(nz4, vmulq_f32(j4, imA));
        float32x4_t dVBX=vmulq_f32(nx4, vmulq_f32(j4, imB));
        float32x4_t dVBZ=vmulq_f32(nz4, vmulq_f32(j4, imB));
        float32x4_t dAngA=vmulq_f32(vmulq_f32(ranA, j4), iiA);
        float32x4_t dAngB=vmulq_f32(vmulq_f32(ranB, j4), iiB);

        uint32_t app[4]; vst1q_u32(app, approach);
        for (int k=0;k<4;++k) {
            if (!app[k] || idxA[k]==UINT32_MAX || idxB[k]==UINT32_MAX) continue;
            if (idxA[k]>=limit || idxB[k]>=limit) continue;
            localDelta[idxA[k]].velX += dVAX[k] * iterationScale; localDelta[idxA[k]].velZ += dVAZ[k] * iterationScale;
            localDelta[idxB[k]].velX -= dVBX[k] * iterationScale; localDelta[idxB[k]].velZ -= dVBZ[k] * iterationScale;
            localDelta[idxA[k]].angVel += dAngA[k] * iterationScale;
            localDelta[idxB[k]].angVel -= dAngB[k] * iterationScale;
        }
    }
}

void XPBDPhysicsSystem::MergeThreadDeltas() {
    if (m_ContactCount == 0 && m_ConstraintCount == 0) return;
    const size_t numThreads = std::min(std::max(JobSystem::Get().NumWorkers(), 1UL), MAX_WORKER_THREADS);
    const size_t limit = m_activeFlatEntities;
    if (limit == 0) return;
    const size_t mergeWorkers = std::min(numThreads, limit);
    if (mergeWorkers <= 1) {
        MergeThreadDeltasRange(0, limit, numThreads);
    } else {
        for (size_t worker = 0; worker < mergeWorkers; ++worker) {
            m_MergeJobContexts[worker] = {this, limit * worker / mergeWorkers,
                                          limit * (worker + 1) / mergeWorkers, numThreads};
            JobSystem::Get().ExecuteRaw(&XPBDPhysicsSystem::RunMergeJob, &m_MergeJobContexts[worker]);
        }
        JobSystem::Get().WaitForAll();
    }
    for (size_t worker = 0; worker < numThreads; ++worker) m_ThreadDeltaActive[worker] = 0;
}

void XPBDPhysicsSystem::RunMergeJob(void* rawContext) {
    const auto& context = *static_cast<MergeJobContext*>(rawContext);
    context.system->MergeThreadDeltasRange(context.beginEntity, context.endEntity, context.activeWorkerCount);
}

void XPBDPhysicsSystem::MergeThreadDeltasRange(size_t beginEntity, size_t endEntity, size_t activeWorkerCount) {
    for (size_t entity = beginEntity; entity < endEntity; ++entity) {
        DeferredDelta& merged = m_DeferredDelta[entity];
        for (size_t worker = 0; worker < activeWorkerCount; ++worker) {
            if (!m_ThreadDeltaActive[worker]) continue;
            DeferredDelta& delta = m_ThreadDeltas[worker][entity];
            if (delta.posX==0 && delta.posZ==0 && delta.velX==0 && delta.velZ==0 && delta.rot==0 && delta.angVel==0) continue;
            merged.posX += delta.posX; merged.posZ += delta.posZ;
            merged.velX += delta.velX; merged.velZ += delta.velZ;
            merged.rot += delta.rot; merged.angVel += delta.angVel;
            delta = {0,0,0,0,0,0};
        }
        if (merged.posX!=0||merged.posZ!=0||merged.velX!=0||merged.velZ!=0||merged.rot!=0||merged.angVel!=0) {
            m_flatPosX[entity] += merged.posX; m_flatPosZ[entity] += merged.posZ;
            m_flatVelX[entity] += merged.velX; m_flatVelZ[entity] += merged.velZ;
            m_flatRot[entity] += merged.rot; m_flatAngVel[entity] += merged.angVel;
            merged = {0,0,0,0,0,0};
        }
    }
}

void XPBDPhysicsSystem::WriteBackToECS(ArchetypeManager& em) {
    auto chunks = em.GetChunks<PositionComponent, VelocityComponent, ColliderComponent>();
    size_t index = 0;
    for (auto* chunk : chunks) {
        if (!chunk || chunk->count == 0 || chunk->posX == nullptr) continue;
        for (size_t entity = 0; entity < chunk->count && index < m_activeFlatEntities; ++entity) {
            chunk->posX[entity] = m_flatPosX[index];
            chunk->posZ[entity] = m_flatPosZ[index];
            chunk->velX[entity] = m_flatVelX[index];
            chunk->velZ[entity] = m_flatVelZ[index];
            if (chunk->rotZ) chunk->rotZ[entity] = m_flatRot[index];
            ++index;
        }
    }
}

// Step
void XPBDPhysicsSystem::Step(ArchetypeManager& em, float dt) {
    using Clock = std::chrono::steady_clock;
    const auto millisSince = [](const Clock::time_point& start) {
        return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
    };
    if (m_TimingEnabled) m_StepTimingStats = {};
    const auto buildFlatStarted = m_TimingEnabled ? Clock::now() : Clock::time_point{};
    BuildFlatArrays(em);
    if (m_TimingEnabled) m_StepTimingStats.buildFlatMs = millisSince(buildFlatStarted);
    const size_t totalEntities = m_activeFlatEntities;
    if (totalEntities == 0) return;
    const size_t requiredWorkers = std::min(std::max(JobSystem::Get().NumWorkers(), 1UL), MAX_WORKER_THREADS);
    for (size_t worker = 0; worker < requiredWorkers; ++worker) {
        if (m_ThreadDeltas[worker].size() < totalEntities)
            m_ThreadDeltas[worker].resize(totalEntities, {0,0,0,0,0,0});
    }
    ++m_FrameCount;
    if (m_WorldPartitionEnabled) UpdateWorldChunks();
    ++m_CurrentStamp;
    if (m_CurrentStamp == 0) { std::fill(m_PairStamp.begin(), m_PairStamp.end(), SIZE_MAX); m_CurrentStamp = 1; }
    m_ContactCount = 0;
    m_ManifoldCacheStats = {};
    m_BroadphaseStats = {};
    m_BroadphaseTimingStats = {};
    const auto setupStarted = m_TimingEnabled ? Clock::now() : Clock::time_point{};
    for (size_t i = 0; i < totalEntities; ++i) {
        float speed = fabsf(m_flatVelX[i]) + fabsf(m_flatVelZ[i]) + fabsf(m_flatAngVel[i]);
        m_IsAwake[i] = (speed > SLEEP_THRESHOLD) ? 1 : 0;
    }
    if (m_TimingEnabled) m_StepTimingStats.setupMs = millisSince(setupStarted);

    const auto broadphaseStarted = m_TimingEnabled ? Clock::now() : Clock::time_point{};
    size_t broadphaseParticipants = 0;
    for (size_t i = 0; i < m_activeFlatEntities; ++i)
        if (m_IsAwake[i] || m_flatInvMass[i] <= 0.0f) ++broadphaseParticipants;
    const bool useGridBroadphase = m_UseGridBroadphase && broadphaseParticipants >= 8192;
    if (useGridBroadphase) {
        m_BVHRoot = -1;
        m_BVHNodes.clear();
        GridBroadphase();
    } else {
        std::vector<int> indices;
        indices.reserve(m_activeFlatEntities);
        // Static colliders are spatial query participants even when no dynamic body is awake.
        // Excluding them made resting or initial dynamic–static overlaps invisible to the BVH.
        for (size_t i = 0; i < m_activeFlatEntities; ++i)
            if (m_IsAwake[i] || m_flatInvMass[i] <= 0.0f) indices.push_back((int)i);
        if (!indices.empty()) BuildBVHMorton(indices); else m_BVHRoot = -1;
    }
    if (m_TimingEnabled) m_StepTimingStats.broadphaseMs = millisSince(broadphaseStarted);

    const auto integrateStarted = m_TimingEnabled ? Clock::now() : Clock::time_point{};
    const float simulationDt = std::clamp(dt, 0.0001f, 0.05f);
    for (size_t i = 0; i < totalEntities; ++i) {
        if (m_flatInvMass[i] <= 0.0f) continue;
        m_flatPosX[i] += m_flatVelX[i] * simulationDt;
        m_flatPosZ[i] += m_flatVelZ[i] * simulationDt;
        m_flatRot[i] += m_flatAngVel[i] * simulationDt;
    }
    if (m_TimingEnabled) m_StepTimingStats.integrateMs = millisSince(integrateStarted);

    if (!useGridBroadphase) {
        CCDPass(simulationDt);
        if (m_BVHRoot != -1) QueryBVHPairsIterative(m_BVHRoot, m_BVHRoot);
    }
    if (m_TimingEnabled) m_StepTimingStats.broadphaseMs += millisSince(integrateStarted) - m_StepTimingStats.integrateMs;
        
    if (m_GPUBroadphase) GPUBroadphaseQuery();

    if (m_ConstraintCount > 0) { auto& js = JobSystem::Get(); const size_t numThreads = std::max(js.NumWorkers(), 1UL);
        for (size_t t = 0; t < numThreads; ++t) js.Execute([this, simulationDt, t]() { SolveConstraintsColored(simulationDt, t); });
        js.WaitForAll(); }

    if (!m_ClothPatches.empty()) SolveCloth(simulationDt, 0);
    if (!m_PendingFractures.empty()) ProcessFractures();
    if (m_ContactCount == 0) {
        m_LastContactCount = 0;
        const auto mergeStarted = m_TimingEnabled ? Clock::now() : Clock::time_point{};
        MergeThreadDeltas();
        if (m_TimingEnabled) m_StepTimingStats.mergeMs = millisSince(mergeStarted);
        const auto writeBackStarted = m_TimingEnabled ? Clock::now() : Clock::time_point{};
        WriteBackToECS(em);
        if (m_TimingEnabled) m_StepTimingStats.writeBackMs = millisSince(writeBackStarted);
        return;
    }

    const auto islandsStarted = m_TimingEnabled ? Clock::now() : Clock::time_point{};
    BuildConstraintGraph();
    if (m_TimingEnabled) m_StepTimingStats.islandsAndGraphMs = millisSince(islandsStarted);
    const auto solveStarted = m_TimingEnabled ? Clock::now() : Clock::time_point{};
    float compliance = 0.0001f / (simulationDt * simulationDt);
    constexpr int maxIter = 3;
    const size_t workerCount = std::min(std::max(JobSystem::Get().NumWorkers(), 1UL), MAX_WORKER_THREADS);
    if (workerCount <= 1 || m_NumColors <= 1) {
        for (size_t color = 0; color < m_NumColors; ++color)
            SolveColorBatch(color, compliance, simulationDt, 0, maxIter);
    } else {
        struct ColorTask { size_t color; size_t offset; size_t count; };
        constexpr size_t kMinContactsPerTask = 4096;
        std::vector<ColorTask> colorTasks;
        for (size_t color = 0; color < m_NumColors; ++color) {
            const size_t count = m_ColorBatches[color].count;
            const size_t parts = std::min(workerCount, std::max<size_t>(1, (count + kMinContactsPerTask - 1) / kMinContactsPerTask));
            for (size_t part = 0; part < parts; ++part) {
                const size_t begin = count * part / parts;
                const size_t end = count * (part + 1) / parts;
                if (begin < end) colorTasks.push_back({color, begin, end - begin});
            }
        }
        const size_t scheduledWorkers = std::min(workerCount, colorTasks.size());
        for (size_t worker = 0; worker < scheduledWorkers; ++worker) {
            JobSystem::Get().Execute([this, &colorTasks, compliance, simulationDt, worker, scheduledWorkers, maxIter]() {
                for (size_t task = worker; task < colorTasks.size(); task += scheduledWorkers) {
                    const ColorTask& work = colorTasks[task];
                    SolveColorBatch(work.color, compliance, simulationDt, worker, maxIter, work.offset, work.count);
                }
            });
        }
        JobSystem::Get().WaitForAll();
    }
    if (m_TimingEnabled) m_StepTimingStats.solveMs = millisSince(solveStarted);
    const auto mergeStarted = m_TimingEnabled ? Clock::now() : Clock::time_point{};
    MergeThreadDeltas();
    if (m_TimingEnabled) m_StepTimingStats.mergeMs = millisSince(mergeStarted);

    const auto writeBackStarted = m_TimingEnabled ? Clock::now() : Clock::time_point{};
    WriteBackToECS(em);
    if (m_TimingEnabled) m_StepTimingStats.writeBackMs = millisSince(writeBackStarted);
    m_LastContactCount = m_ContactCount;
}

size_t XPBDPhysicsSystem::GetManifoldCount() const { return m_LastContactCount; }
size_t XPBDPhysicsSystem::GetMaxContactColorBatch() const {
    size_t maxCount = 0;
    for (const ColorBatch& batch : m_ColorBatches) maxCount = std::max(maxCount, static_cast<size_t>(batch.count));
    return maxCount;
}
uint64_t XPBDPhysicsSystem::MakeKey(EntityID a, EntityID b) const { if (a > b) std::swap(a, b); return ((uint64_t)a << 32) | b; }

bool XPBDPhysicsSystem::Raycast(float ox, float oz, float dx, float dz, float maxDist, RayHit& outHit, CollisionMask mask) const {
    if (m_BVHRoot < 0) return false;
    float len = sqrtf(dx*dx+dz*dz); if (len < 1e-8f) return false;
    dx /= len; dz /= len; float closest = maxDist; bool hit = false;
    m_BVHStack.clear(); m_BVHStack.push_back(m_BVHRoot);
    while (!m_BVHStack.empty()) {
        int idx = m_BVHStack.back(); m_BVHStack.pop_back();
        if (idx < 0 || idx >= (int)m_BVHNodes.size()) continue;
        const BVHNode& node = m_BVHNodes[idx];
        float tminX = (node.minX - ox) / (dx != 0 ? dx : 1e-8f);
        float tmaxX = (node.maxX - ox) / (dx != 0 ? dx : 1e-8f);
        if (tminX > tmaxX) std::swap(tminX, tmaxX);
        float tminZ = (node.minZ - oz) / (dz != 0 ? dz : 1e-8f);
        float tmaxZ = (node.maxZ - oz) / (dz != 0 ? dz : 1e-8f);
        if (tminZ > tmaxZ) std::swap(tminZ, tmaxZ);
        float tEnter = std::max(tminX, tminZ), tExit = std::min(tmaxX, tmaxZ);
        if (tEnter > tExit || tExit < 0 || tEnter > closest) continue;
        if (node.isLeaf) {
            if (node.entityIdx < 0) continue;
            if ((m_EntityLayers[node.entityIdx] & mask) == 0) continue;
            float cx = m_flatPosX[node.entityIdx] - ox, cz = m_flatPosZ[node.entityIdx] - oz;
            float r = m_flatRadius[node.entityIdx];
            float proj = cx*dx + cz*dz; if (proj < 0 || proj > closest) continue;
            float perpSq = (cx*cx+cz*cz) - proj*proj;
            if (perpSq <= r*r) {
                float halfCord = sqrtf(r*r - perpSq);
                float tHit = proj - halfCord;
                if (tHit >= 0 && tHit < closest) {
                    closest = tHit; outHit.entityIdx = node.entityIdx; outHit.distance = tHit;
                    float hx = ox + dx*tHit, hz = oz + dz*tHit;
                    float nx = hx - m_flatPosX[node.entityIdx], nz = hz - m_flatPosZ[node.entityIdx];
                    float nLen = sqrtf(nx*nx+nz*nz); if (nLen > 1e-8f) { nx /= nLen; nz /= nLen; }
                    outHit.normalX = nx; outHit.normalZ = nz; hit = true;
                }
            }
        } else { m_BVHStack.push_back(node.left); m_BVHStack.push_back(node.right); }
    }
    return hit;
}
void XPBDPhysicsSystem::SetEntityLayer(uint32_t flatIdx, CollisionMask layer) { if (flatIdx < m_EntityLayers.size()) m_EntityLayers[flatIdx] = layer; }
CollisionMask XPBDPhysicsSystem::GetEntityLayer(uint32_t flatIdx) const { return (flatIdx < m_EntityLayers.size()) ? m_EntityLayers[flatIdx] : COLLISION_LAYER_NONE; }

std::vector<uint32_t> XPBDPhysicsSystem::OverlapSphere(float centerX, float centerZ, float radius,
                                                        CollisionMask mask) const {
    std::vector<uint32_t> results;
    if (radius < 0.0f) return results;
    const float queryRadiusSquared = radius * radius;
    for (size_t index = 0; index < m_activeFlatEntities; ++index) {
        if ((m_EntityLayers[index] & mask) == 0) continue;
        const float dx = m_flatPosX[index] - centerX;
        const float dz = m_flatPosZ[index] - centerZ;
        const float combinedRadius = radius + m_flatRadius[index];
        if (dx * dx + dz * dz <= combinedRadius * combinedRadius) results.push_back(static_cast<uint32_t>(index));
    }
    return results;
}

void XPBDPhysicsSystem::FractureEntity(uint32_t entityIdx, int numPieces, const std::vector<FracturePiece>& pieces) {
    if (numPieces <= 0 || pieces.empty() || static_cast<size_t>(numPieces) > pieces.size()) return;
    FractureRecord record;
    record.entityIdx = entityIdx;
    record.numPieces = numPieces;
    record.pieces.assign(pieces.begin(), pieces.begin() + numPieces);
    record.activationFrame = m_FrameCount + 1;
    m_PendingFractures.push_back(std::move(record));
}

uint32_t XPBDPhysicsSystem::AddClothPatch(const std::vector<ClothParticle>& particles, float stiffness, float damping) {
    if (particles.size() < 2 || stiffness <= 0.0f || damping < 0.0f) return UINT32_MAX;
    ClothPatch patch;
    patch.particles = particles;
    patch.stiffness = std::clamp(stiffness, 0.0f, 1.0f);
    patch.damping = damping;
    m_ClothPatches.push_back(std::move(patch));
    return static_cast<uint32_t>(m_ClothPatches.size() - 1);
}

std::vector<uint8_t> XPBDPhysicsSystem::SerializePhysicsState() const {
    constexpr uint32_t kStateMagic = 0x44504258U; // XPBD
    constexpr uint32_t kStateVersion = 1U;
    std::vector<uint8_t> bytes;
    bytes.reserve(sizeof(uint32_t) * 3 + m_activeFlatEntities * sizeof(float) * 6);
    const auto append = [&bytes](const auto& value) {
        const uint8_t* raw = reinterpret_cast<const uint8_t*>(&value);
        bytes.insert(bytes.end(), raw, raw + sizeof(value));
    };
    append(kStateMagic);
    append(kStateVersion);
    const uint32_t count = static_cast<uint32_t>(m_activeFlatEntities);
    append(count);
    for (size_t index = 0; index < m_activeFlatEntities; ++index) {
        append(m_flatPosX[index]); append(m_flatPosZ[index]);
        append(m_flatVelX[index]); append(m_flatVelZ[index]);
        append(m_flatRot[index]); append(m_flatAngVel[index]);
    }
    return bytes;
}

void XPBDPhysicsSystem::DeserializePhysicsState(const std::vector<uint8_t>& data) {
    constexpr uint32_t kStateMagic = 0x44504258U;
    constexpr uint32_t kStateVersion = 1U;
    size_t offset = 0;
    const auto read = [&data, &offset](auto& value) {
        if (offset + sizeof(value) > data.size()) return false;
        std::memcpy(&value, data.data() + offset, sizeof(value));
        offset += sizeof(value);
        return true;
    };
    uint32_t magic = 0, version = 0, count = 0;
    if (!read(magic) || !read(version) || !read(count) || magic != kStateMagic || version != kStateVersion ||
        count > m_maxFlatEntities) return;
    for (uint32_t index = 0; index < count; ++index) {
        if (!read(m_flatPosX[index]) || !read(m_flatPosZ[index]) || !read(m_flatVelX[index]) ||
            !read(m_flatVelZ[index]) || !read(m_flatRot[index]) || !read(m_flatAngVel[index])) return;
    }
    m_activeFlatEntities = count;
    m_FlatArraysValid = true;
}

void XPBDPhysicsSystem::EnableGPUBroadphase(bool enable) { m_GPUBroadphase = enable; }

void XPBDPhysicsSystem::ProcessFractures() {
    std::vector<FractureRecord> remaining;
    remaining.reserve(m_PendingFractures.size());
    for (const FractureRecord& record : m_PendingFractures) {
        if (record.activationFrame > m_FrameCount) {
            remaining.push_back(record);
            continue;
        }
        if (record.entityIdx >= m_activeFlatEntities) continue;
        const float sourceX = m_flatPosX[record.entityIdx];
        const float sourceZ = m_flatPosZ[record.entityIdx];
        const float sourceRadius = m_flatRadius[record.entityIdx];
        const float sourceVelocityX = m_flatVelX[record.entityIdx];
        const float sourceVelocityZ = m_flatVelZ[record.entityIdx];
        for (const FracturePiece& piece : record.pieces) {
            if (piece.entityIdx >= m_activeFlatEntities || piece.entityIdx == record.entityIdx) continue;
            m_flatPosX[piece.entityIdx] = sourceX + piece.offsetX;
            m_flatPosZ[piece.entityIdx] = sourceZ + piece.offsetZ;
            m_flatVelX[piece.entityIdx] = sourceVelocityX;
            m_flatVelZ[piece.entityIdx] = sourceVelocityZ;
            m_flatRadius[piece.entityIdx] = std::max(sourceRadius * piece.sizeScale, 0.001f);
            m_flatInvMass[piece.entityIdx] = std::max(m_flatInvMass[piece.entityIdx], 1.0f);
            m_IsAwake[piece.entityIdx] = 1;
        }
        m_flatRadius[record.entityIdx] = 0.0f;
        m_flatInvMass[record.entityIdx] = 0.0f;
        m_IsAwake[record.entityIdx] = 0;
    }
    m_PendingFractures.swap(remaining);
}

void XPBDPhysicsSystem::UpdateWorldChunks() {
    constexpr float kWorldSize = 200.0f;
    const float chunkSize = kWorldSize / static_cast<float>(CHUNK_DIM);
    if (m_WorldChunks.size() != CHUNK_DIM * CHUNK_DIM) {
        m_WorldChunks.resize(CHUNK_DIM * CHUNK_DIM);
        for (int z = 0; z < CHUNK_DIM; ++z) for (int x = 0; x < CHUNK_DIM; ++x) {
            WorldChunk& chunk = m_WorldChunks[z * CHUNK_DIM + x];
            chunk.minX = x * chunkSize; chunk.maxX = (x + 1) * chunkSize;
            chunk.minZ = z * chunkSize; chunk.maxZ = (z + 1) * chunkSize;
            chunk.active = false;
        }
    }
    for (WorldChunk& chunk : m_WorldChunks) chunk.active = false;
    for (size_t index = 0; index < m_activeFlatEntities; ++index) {
        if (!m_IsAwake[index] && m_flatInvMass[index] > 0.0f) continue;
        const int x = std::clamp(static_cast<int>(m_flatPosX[index] / chunkSize), 0, CHUNK_DIM - 1);
        const int z = std::clamp(static_cast<int>(m_flatPosZ[index] / chunkSize), 0, CHUNK_DIM - 1);
        const int chunkIndex = z * CHUNK_DIM + x;
        m_ChunkAssignment[index] = chunkIndex;
        m_WorldChunks[chunkIndex].active = true;
    }
}

void XPBDPhysicsSystem::GPUBroadphaseQuery() {
    // GPU dispatch is platform-dependent. The CPU BVH is the deterministic authority
    // and remains active whenever a compute backend has not supplied a validated result.
    if (m_BVHRoot != -1) QueryBVHPairsIterative(m_BVHRoot, m_BVHRoot);
}

} // namespace
