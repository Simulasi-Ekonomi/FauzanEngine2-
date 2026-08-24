#pragma once
#include "Core/ECS/ArchetypeManager.h"
#include "Core/Math/MathTypes.h"
#include "Core/Math/SimdCompat.h"
#include <vector>
#include <array>
#include <atomic>
#include <cstdint>
#include <unordered_map>

namespace NeoEngine {

constexpr size_t PHYS_ENTITIES_MAX = 131072;
constexpr size_t MAX_CONTACTS = 262144;
constexpr size_t MAX_CONSTRAINTS = 32768;
constexpr size_t STAMP_SIZE = 524288;
constexpr size_t STAMP_SHIFT = 45;
constexpr size_t MAX_MANIFOLDS = 262144;
constexpr float FAT_MARGIN = 0.05f;
constexpr float SLEEP_THRESHOLD = 0.005f;
constexpr float RESTITUTION = 0.1f;
constexpr float FRICTION_COEFF = 0.5f;
constexpr int BVH_REBUILD_INTERVAL = 60;
constexpr float BVH_ROTATE_COST_RATIO = 0.3f;
constexpr int MAX_MANIFOLD_AGE = 60;
constexpr int PAIR_PROBE_LIMIT = 16;
constexpr float CCD_VELOCITY_THRESHOLD_SQ = 25.0f;
constexpr size_t CONTACT_BLOCK_SIZE = 32;
constexpr size_t MAX_GRAPH_COLORS = 64;
constexpr size_t MAX_WORKER_THREADS = 16;
constexpr size_t DENSE_MANIFOLD_SHARDS = 32;
constexpr size_t DENSE_MANIFOLD_SHARD_BITS = 5;
constexpr float DENSE_GRID_CELL_DIAMETER_SCALE = 1.0f;
constexpr size_t DENSE_MANIFOLD_TOTAL = MAX_MANIFOLDS + MAX_MANIFOLDS / 2;
constexpr size_t DENSE_MANIFOLD_SHARD_STAMPS = STAMP_SIZE / DENSE_MANIFOLD_SHARDS;
constexpr size_t DENSE_MANIFOLD_SHARD_CAPACITY = DENSE_MANIFOLD_TOTAL / DENSE_MANIFOLD_SHARDS;
constexpr size_t DENSE_MANIFOLD_PROBE_LIMIT = 64;
constexpr uint32_t DENSE_MANIFOLD_INDEX_FLAG = 0x80000000u;

using CollisionMask = uint32_t;
constexpr CollisionMask COLLISION_LAYER_NONE = 0;
constexpr CollisionMask COLLISION_LAYER_DEFAULT = 1 << 0;
constexpr CollisionMask COLLISION_LAYER_STATIC = 1 << 1;
constexpr CollisionMask COLLISION_LAYER_DYNAMIC = 1 << 2;
constexpr CollisionMask COLLISION_LAYER_TRIGGER = 1 << 3;

struct alignas(64) ContactBlock {
    uint32_t idxA[CONTACT_BLOCK_SIZE];
    uint32_t idxB[CONTACT_BLOCK_SIZE];
    float posAX[CONTACT_BLOCK_SIZE], posAZ[CONTACT_BLOCK_SIZE];
    float posBX[CONTACT_BLOCK_SIZE], posBZ[CONTACT_BLOCK_SIZE];
    float radA[CONTACT_BLOCK_SIZE], radB[CONTACT_BLOCK_SIZE];
    float invMassA[CONTACT_BLOCK_SIZE], invMassB[CONTACT_BLOCK_SIZE];
    float lambdaN[CONTACT_BLOCK_SIZE], lambdaT[CONTACT_BLOCK_SIZE];
    float rotA[CONTACT_BLOCK_SIZE], rotB[CONTACT_BLOCK_SIZE];
    float invInertiaA[CONTACT_BLOCK_SIZE], invInertiaB[CONTACT_BLOCK_SIZE];
    float contactOffsetAX[CONTACT_BLOCK_SIZE], contactOffsetAZ[CONTACT_BLOCK_SIZE];
    float contactOffsetBX[CONTACT_BLOCK_SIZE], contactOffsetBZ[CONTACT_BLOCK_SIZE];
    uint32_t manifoldIndex[CONTACT_BLOCK_SIZE];
    uint32_t manifoldGeneration[CONTACT_BLOCK_SIZE];
};

struct SolveLane {
    float posAX[4], posAZ[4], posBX[4], posBZ[4];
    float radA[4], radB[4];
    float invMassA[4], invMassB[4];
    float lambdaN[4], lambdaT[4];
    uint32_t idxA[4], idxB[4];
    float rotA[4], rotB[4];
    float invInertiaA[4], invInertiaB[4];
    float rAX[4], rAZ[4], rBX[4], rBZ[4];
    uint32_t manifoldIndex[4], manifoldGeneration[4];
};

struct PersistentContact { uint32_t idxA, idxB; uint32_t lastSeenFrame; bool active; };
struct Manifold { uint32_t idxA, idxB; float lambdaN, lambdaT; };
struct ManifoldCacheStats {
    size_t hits = 0;
    size_t createAttempts = 0;
    size_t createSuccesses = 0;
    size_t rejections = 0;
    size_t probeSteps = 0;
    size_t maxProbeDepth = 0;
};
struct BroadphaseStats { size_t candidatePairs = 0; size_t occupiedCells = 0; size_t activeCells = 0; };
struct BroadphaseTimingStats {
    double boundsMs = 0.0;
    double gridBuildMs = 0.0;
    double pairTraversalMs = 0.0;
    double gatherMs = 0.0;
    double emitMs = 0.0;
};
struct StepTimingStats {
    double buildFlatMs = 0.0;
    double setupMs = 0.0;
    double broadphaseMs = 0.0;
    double integrateMs = 0.0;
    double islandsAndGraphMs = 0.0;
    double solveMs = 0.0;
    double mergeMs = 0.0;
    double writeBackMs = 0.0;
};
struct BVHNode { float minX, minZ, maxX, maxZ; float cachedCost; int left, right, parent; int entityIdx; bool isLeaf; };
struct IslandRange { uint32_t start, count; };
struct DeferredDelta { float posX, posZ, velX, velZ; float rot, angVel; };
struct RayHit { uint32_t entityIdx; float distance; float normalX, normalZ; };
struct ColorBatch { uint32_t start; uint32_t count; };
struct GridContactCandidate { uint32_t idxA, idxB; float dx, dz, d2, sumR, invDist; };

enum class ConstraintType : uint8_t {
    Distance = 0,
    Hinge = 1,
    MotorizedHinge = 2,
    Breakable = 3,
    Cloth = 4,
    ConeTwist = 5,
    Prismatic = 6,
    Fixed = 7,
    Spherical = 8,
};

struct Constraint {
    uint32_t idxA, idxB;
    ConstraintType type;
    float anchorAX, anchorAZ, anchorBX, anchorBZ;
    float axisX, axisZ;
    float limitLow, limitHigh;
    float lambda[3];
    float breakThreshold;
    float motorSpeed;
    float stiffness = 1000.0f;
    float maxForce = 0.0f;
    uint32_t age;
    bool active;
};

struct FracturePiece { uint32_t entityIdx; float offsetX, offsetZ; float sizeScale; };
struct ClothParticle { uint32_t entityIdx; float uvX, uvZ; float invMass; };

class XPBDPhysicsSystem {
public:
    XPBDPhysicsSystem();
    void Step(ArchetypeManager& em, float dt);
    size_t GetManifoldCount() const;
    size_t GetManifoldCacheSize() const { return static_cast<size_t>(m_ManifoldCount); }
    size_t GetContactColorCount() const { return m_NumColors; }
    size_t GetMaxContactColorBatch() const;
    const ManifoldCacheStats& GetManifoldCacheStats() const { return m_ManifoldCacheStats; }
    const BroadphaseStats& GetBroadphaseStats() const { return m_BroadphaseStats; }
    const BroadphaseTimingStats& GetBroadphaseTimingStats() const { return m_BroadphaseTimingStats; }
    void SetTimingEnabled(bool enabled) { m_TimingEnabled = enabled; }
    void SetProbeMetricsEnabled(bool enabled) { m_ProbeMetricsEnabled = enabled; }
    const StepTimingStats& GetStepTimingStats() const { return m_StepTimingStats; }

    uint32_t AddHingeJoint(uint32_t idxA, uint32_t idxB,
                           float anchorAX, float anchorAZ,
                           float anchorBX, float anchorBZ,
                           float axisX, float axisZ,
                           float motorSpeed = 0.0f,
                           float breakThreshold = 0.0f);
    uint32_t AddConeTwistJoint(uint32_t idxA, uint32_t idxB,
                              float anchorAX, float anchorAZ,
                              float anchorBX, float anchorBZ,
                              float limitLow, float limitHigh);
    uint32_t AddPrismaticJoint(uint32_t idxA, uint32_t idxB,
                              float axisX, float axisZ,
                              float limitLow, float limitHigh);
    void RemoveConstraint(uint32_t constraintId);
    void FractureEntity(uint32_t entityIdx, int numPieces,
                        const std::vector<FracturePiece>& pieces);
    uint32_t AddClothPatch(const std::vector<ClothParticle>& particles,
                           float stiffness, float damping);
    std::vector<uint8_t> SerializePhysicsState() const;
    void DeserializePhysicsState(const std::vector<uint8_t>& data);
    void EnableGPUBroadphase(bool enable);

    bool Raycast(float originX, float originZ, float dirX, float dirZ, float maxDist, RayHit& outHit, CollisionMask mask = 0xFFFFFFFF) const;
    void SetEntityLayer(uint32_t flatIdx, CollisionMask layer);
    CollisionMask GetEntityLayer(uint32_t flatIdx) const;

    uint32_t AddDistanceJoint(uint32_t a, uint32_t b, float minDist, float maxDist, float stiffness=1000.0f);
    uint32_t AddFixedJoint(uint32_t a, uint32_t b, float anchorAX, float anchorAZ, float anchorBX, float anchorBZ);
    uint32_t AddSphericalJoint(uint32_t a, uint32_t b, float anchorAX, float anchorAZ, float anchorBX, float anchorBZ, float limitLow, float limitHigh);
    void SetConstraintDrive(uint32_t id, float targetVelocity, float maxForce);
    std::vector<uint32_t> OverlapSphere(float centerX, float centerZ, float radius, CollisionMask mask = 0xFFFFFFFF) const;
private:
    std::vector<float> m_flatPosX, m_flatPosZ;
    std::vector<float> m_flatVelX, m_flatVelZ;
    std::vector<float> m_flatRadius, m_flatInvMass;
    std::vector<uint32_t> m_flatEntityIDs;
    std::vector<uint8_t> m_IsAwake, m_IsAwakePrev;
    std::vector<DeferredDelta> m_DeferredDelta;
    std::vector<std::vector<DeferredDelta>> m_ThreadDeltas;
    std::vector<uint8_t> m_ThreadDeltaActive;
    struct MergeJobContext {
        XPBDPhysicsSystem* system = nullptr;
        size_t beginEntity = 0;
        size_t endEntity = 0;
        size_t activeWorkerCount = 0;
    };
    std::vector<MergeJobContext> m_MergeJobContexts;
    std::vector<uint32_t> m_UF_Parent, m_UF_ParentPrev;
    std::vector<int> m_LeafNode;
    std::vector<CollisionMask> m_EntityLayers;

    std::vector<float> m_flatRot;
    std::vector<float> m_flatAngVel;
    std::vector<float> m_flatInvInertia;

    size_t m_activeFlatEntities = 0;
    size_t m_maxFlatEntities = 0;
    uint64_t m_EcsPhysicsRevision = 0;
    bool m_FlatArraysValid = false;

    std::vector<ContactBlock> m_ContactBlocks;
    size_t m_ContactCount = 0;

    std::vector<PersistentContact> m_PersistentContacts;
    std::vector<Manifold> m_Manifolds;
    std::vector<uint32_t> m_ManifoldStamp;
    std::vector<Manifold> m_DenseShardManifolds;
    std::vector<uint32_t> m_DenseShardStamp;
    std::array<size_t, DENSE_MANIFOLD_SHARDS> m_DenseShardCounts{};
    std::array<std::vector<uint32_t>, DENSE_MANIFOLD_SHARDS> m_DenseShardContacts;
    std::array<ManifoldCacheStats, DENSE_MANIFOLD_SHARDS> m_DenseShardStats{};
    int m_ManifoldCount = 0;
    uint32_t m_ManifoldGeneration = 1;
    ManifoldCacheStats m_ManifoldCacheStats;
    BroadphaseStats m_BroadphaseStats;
    BroadphaseTimingStats m_BroadphaseTimingStats;
    StepTimingStats m_StepTimingStats;
    bool m_TimingEnabled = false;
    bool m_ProbeMetricsEnabled = false;
    std::vector<size_t> m_PairStamp;
    std::vector<uint64_t> m_PairKeys;
    size_t m_CurrentStamp = 1;
    uint32_t m_FrameCount = 0;

    std::vector<IslandRange> m_IslandRanges;
    std::vector<uint32_t> m_IslandSizes;
    std::vector<uint32_t> m_IslandContactBlock;
    std::vector<uint32_t> m_IslandRoots;
    std::vector<uint32_t> m_RootToIslandIndex;
    std::vector<uint32_t> m_IslandDirty;
    std::vector<uint32_t> m_IslandRootsPrev, m_IslandSizesPrev;
    std::vector<size_t> m_IslandOrder;
    size_t m_NumIslands = 0;

    std::vector<ColorBatch> m_ColorBatches;
    std::vector<uint32_t> m_SortedContactIndices;
    std::vector<int8_t> m_EntityColor;
    std::vector<uint8_t> m_ContactColors;
    std::vector<uint64_t> m_EntityColorMasks;
    size_t m_NumColors = 0;

    std::vector<BVHNode> m_BVHNodes;
    int m_BVHRoot = -1;
    bool m_BVHInitialized = false;
    mutable std::vector<int> m_BVHStack;
    std::vector<int> m_PostOrderCache;

    std::vector<std::vector<SolveLane>> m_SolveBuffers;
    size_t m_LastContactCount = 0;

    std::vector<Constraint> m_Constraints;
    size_t m_ConstraintCount = 0;
    std::vector<uint32_t> m_BrokenConstraints;

    struct FractureRecord { uint32_t entityIdx; int numPieces; std::vector<FracturePiece> pieces; uint32_t activationFrame; };
    std::vector<FractureRecord> m_PendingFractures;

    struct ClothPatch { std::vector<ClothParticle> particles; float stiffness; float damping; };
    std::vector<ClothPatch> m_ClothPatches;

    struct WorldChunk { float minX, minZ, maxX, maxZ; bool active; };
    std::vector<WorldChunk> m_WorldChunks;
    static constexpr int CHUNK_DIM = 8;
    bool m_WorldPartitionEnabled = true;
    std::vector<int> m_ChunkAssignment;

    bool m_GPUBroadphase = false;
    std::vector<float> m_flatPosXPrev, m_flatPosZPrev;

    std::vector<int8_t> m_ConstraintColor;
    std::vector<ColorBatch> m_ConstraintColorBatches;
    // Spatial Hash Grid untuk dynamic-dynamic broadphase
    bool m_UseGridBroadphase = true;
    float m_GridCellSize = 2.0f;
    std::unordered_map<uint64_t, std::vector<uint32_t>> m_Grid;
    std::vector<int> m_DenseGridHeads;
    std::vector<int> m_DenseGridNext;
    std::vector<uint8_t> m_DenseGridActiveCells;
    std::vector<size_t> m_DenseGridActiveCellList;
    std::vector<std::vector<GridContactCandidate>> m_GridContactCandidates;
    std::vector<size_t> m_GridCandidatePairCounts;
    struct GridGatherJobContext {
        XPBDPhysicsSystem* system = nullptr;
        size_t beginActiveCell = 0;
        size_t endActiveCell = 0;
        int width = 0;
        int height = 0;
        int minCellX = 0;
        int minCellZ = 0;
        size_t workerId = 0;
    };
    struct GridStageJobContext {
        XPBDPhysicsSystem* system = nullptr;
        size_t workerId = 0;
        size_t contactStart = 0;
    };
    struct DenseShardAssociateContext {
        XPBDPhysicsSystem* system = nullptr;
        size_t shard = 0;
    };
    std::vector<GridGatherJobContext> m_GridGatherContexts;
    std::vector<GridStageJobContext> m_GridStageContexts;
    std::array<DenseShardAssociateContext, DENSE_MANIFOLD_SHARDS> m_DenseShardAssociateContexts{};
    void GridBroadphase();
    void GatherDenseGridContacts(size_t beginActiveCell, size_t endActiveCell, int width, int height,
                                 int minCellX, int minCellZ, size_t workerId);
    static void RunGridGatherJob(void* rawContext);
    void StageDenseGridContactPayloads(size_t workerId, size_t contactStart);
    static void RunGridStageJob(void* rawContext);
    void AssociateStagedDenseGridManifolds(size_t workerCount);
    void AssociateDenseManifoldShard(size_t shard);
    static void RunDenseShardAssociateJob(void* rawContext);
    uint64_t GridKey(int cx, int cz) const { return ((uint64_t)(uint32_t)cx << 32) | (uint32_t)cz; }

    void BuildFlatArrays(ArchetypeManager& em);
    void WriteBackToECS(ArchetypeManager& em);
    void RefitNode(int nodeIdx);
    void RefitBVH();
    void RotateBVH(int nodeIdx);
    void QueryBVHPairsIterative(int rootA, int rootB);
    void BuildBVHIterative(std::vector<int>& indices);
    void BuildBVHMorton(std::vector<int>& indices);
    void CCDPass(float dt);
    void CCDQueryBVH(int nodeIdx, uint32_t entityIdx, float dt);
    int FindOrCreateManifold(uint32_t a, uint32_t b, bool& cacheHit);
    void EvictManifolds();
    bool PairStampExists(uint64_t key);
    void EmitContact(uint32_t flatIdxA, uint32_t flatIdxB, float dx, float dz, float d2, float sumR, float invDist,
                     bool pairAlreadyUnique = false, bool indicesCanonical = false);
    void BuildLocalIslands();
    void BuildConstraintGraph();
    void SolveColorBatch(size_t colorIdx, float compliance, float dt, size_t threadId, uint32_t iterationWeight = 1,
                         size_t contactOffset = 0, size_t contactCount = static_cast<size_t>(-1));
    void SolveConstraintsColored(float dt, size_t threadId);
    void SolveCloth(float dt, size_t threadId);
    void MergeThreadDeltas();
    void MergeThreadDeltasRange(size_t beginEntity, size_t endEntity, size_t activeWorkerCount);
    static void RunMergeJob(void* rawContext);
    void ProcessFractures();
    void UpdateWorldChunks();
    void GPUBroadphaseQuery();
    uint64_t MakeKey(EntityID a, EntityID b) const;
    static inline size_t HashIndex(uint64_t key);
};

} // namespace
