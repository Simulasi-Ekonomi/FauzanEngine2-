#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "../Core/ECS/EntityManager.h"

namespace NeoEngine {

struct WorldActor {
    EntityID id;
    std::string name;
    std::string type; // "tree", "rock", "building", "npc", "player"
    float posX, posY, posZ;
    float rotX, rotY, rotZ;
    float scaleX, scaleY, scaleZ;
    bool visible = true;
};

class NeoWorld {
public:
    NeoWorld();
    ~NeoWorld();

    EntityID SpawnActor(const std::string& name, const std::string& type,
                        float x, float y, float z);
    void DestroyActor(EntityID id);
    WorldActor* GetActor(EntityID id);
    void SetActorTransform(EntityID id, float px, float py, float pz,
                           float rx, float ry, float rz,
                           float sx, float sy, float sz);

    size_t GetActorCount() const { return m_Actors.size(); }
    void Clear();
    void Update(float deltaTime);

    // Serialisasi scene untuk editor
    std::string ToJSON() const;

private:
    EntityManager m_EntityManager;
    std::unordered_map<EntityID, WorldActor> m_Actors;
};

} // namespace NeoEngine
