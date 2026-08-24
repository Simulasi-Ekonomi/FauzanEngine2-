#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "../Core/ECS/EntityManager.h"
#include "SpatialGrid.h"

namespace NeoEngine {

using EntityID = uint32_t;

struct WorldActor {
    EntityID id;
    std::string name;
    std::string type;
    float posX=0, posY=0, posZ=0;
    float rotX=0, rotY=0, rotZ=0;
    float scaleX=1, scaleY=1, scaleZ=1;
    float viewDistance = 100.0f;   // Jarak pandang aktor
    bool visible=true;
    bool alive=true;
};

class NeoWorld {
public:
    NeoWorld(float worldSize = 500.0f);
    ~NeoWorld();

    EntityID SpawnActor(const std::string& name, const std::string& type,
                        float x, float y, float z);
    void DestroyActor(EntityID id);
    WorldActor* GetActor(EntityID id);
    size_t GetActorCount() const;
    void Clear();
    
    void Update(float deltaTime);
    
    // Spatial query dengan view distance terbatas
    void QuerySphere(float x, float y, float z, float radius,
                     std::vector<EntityID>& outResults) const;

private:
    static constexpr size_t POOL_SIZE = 10000;
    float m_WorldSize;
    std::vector<WorldActor> m_Pool;
    std::vector<size_t> m_FreeIndices;
    SpatialGrid m_SpatialGrid;
    
    static constexpr float CELL_SIZE = 5.0f;
    static constexpr float DEFAULT_VIEW_DISTANCE = 100.0f;
};

} // namespace NeoEngine
