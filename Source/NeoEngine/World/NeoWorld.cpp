#include "NeoWorld.h"
#include <cmath>
#include <android/log.h>
#define LOG_TAG "NeoWorld"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
namespace NeoEngine {
NeoWorld::NeoWorld(float worldSize) : m_WorldSize(worldSize), m_SpatialGrid(worldSize, CELL_SIZE) {
    m_Pool.resize(POOL_SIZE);
    m_FreeIndices.reserve(POOL_SIZE);
    for (size_t i = 0; i < POOL_SIZE; ++i) m_FreeIndices.push_back(POOL_SIZE - 1 - i);
}
NeoWorld::~NeoWorld() { Clear(); }
EntityID NeoWorld::SpawnActor(const std::string& name, const std::string& type, float x, float y, float z) {
    if (m_FreeIndices.empty()) return 0;
    size_t idx = m_FreeIndices.back();
    m_FreeIndices.pop_back();
    WorldActor& a = m_Pool[idx];
    a.id = idx + 1;
    a.name = name; a.type = type;
    a.posX = x; a.posY = y; a.posZ = z;
    a.alive = true;
    return a.id;
}
void NeoWorld::DestroyActor(EntityID id) {
    if (id == 0 || id > POOL_SIZE) return;
    WorldActor& a = m_Pool[id - 1];
    if (!a.alive) return;
    a.alive = false;
    m_FreeIndices.push_back(id - 1);
}
WorldActor* NeoWorld::GetActor(EntityID id) {
    if (id == 0 || id > POOL_SIZE) return nullptr;
    WorldActor& a = m_Pool[id - 1];
    return a.alive ? &a : nullptr;
}
size_t NeoWorld::GetActorCount() const { return POOL_SIZE - m_FreeIndices.size(); }
void NeoWorld::Clear() {
    for (auto& a : m_Pool) a.alive = false;
    m_FreeIndices.clear();
    for (size_t i = 0; i < POOL_SIZE; ++i) m_FreeIndices.push_back(POOL_SIZE - 1 - i);
}
void NeoWorld::Update(float deltaTime) {
    m_SpatialGrid.Clear();
    for (size_t i = 0; i < POOL_SIZE; ++i) {
        if (m_Pool[i].alive) m_SpatialGrid.Insert(m_Pool[i].id, m_Pool[i].posX, m_Pool[i].posY, m_Pool[i].posZ);
    }
    m_SpatialGrid.ForEachNonEmptyCell([this](int cellIdx, const EntityID* entities, int cnt) {
        m_SpatialGrid.ForEachPairInCellAndNeighbors(cellIdx, [this](EntityID a, EntityID b) {
            if (a == 0 || b == 0 || a > POOL_SIZE || b > POOL_SIZE) return;
            WorldActor& actorA = m_Pool[a - 1];
            WorldActor& actorB = m_Pool[b - 1];
            if (!actorA.alive || !actorB.alive) return;
            float dx = actorA.posX - actorB.posX;
            float dy = actorA.posY - actorB.posY;
            float dz = actorA.posZ - actorB.posZ;
            float dist2 = dx*dx + dy*dy + dz*dz;
            if (dist2 < 4.0f && dist2 > 0.0001f) {
                float dist = std::sqrt(dist2);
                float overlap = 2.0f - dist;
                float nx = dx / dist;
                float ny = dy / dist;
                float nz = dz / dist;
                actorA.posX += nx * overlap * 0.5f;
                actorA.posY += ny * overlap * 0.5f;
                actorA.posZ += nz * overlap * 0.5f;
                actorB.posX -= nx * overlap * 0.5f;
                actorB.posY -= ny * overlap * 0.5f;
                actorB.posZ -= nz * overlap * 0.5f;
            }
        });
    });
}
} // namespace NeoEngine
