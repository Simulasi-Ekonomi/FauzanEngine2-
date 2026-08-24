#include "NeoWorld.h"
#include <sstream>
#include <cmath>
#include <android/log.h>

#define LOG_TAG "NeoWorld"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

NeoWorld::NeoWorld() = default;
NeoWorld::~NeoWorld() { Clear(); }

EntityID NeoWorld::SpawnActor(const std::string& name, const std::string& type,
                               float x, float y, float z) {
    EntityID id = m_EntityManager.CreateEntity();
    if (id == INVALID_ENTITY) {
        LOGI("Failed to spawn actor: entity pool full");
        return INVALID_ENTITY;
    }
    WorldActor actor;
    actor.id = id;
    actor.name = name;
    actor.type = type;
    actor.posX = x; actor.posY = y; actor.posZ = z;
    actor.rotX = 0; actor.rotY = 0; actor.rotZ = 0;
    actor.scaleX = 1; actor.scaleY = 1; actor.scaleZ = 1;
    m_Actors[id] = actor;
    LOGI("Spawned actor: %s (type=%s) at (%.1f, %.1f, %.1f) [id=%u]",
         name.c_str(), type.c_str(), x, y, z, id);
    return id;
}

void NeoWorld::DestroyActor(EntityID id) {
    m_Actors.erase(id);
    m_EntityManager.DestroyEntity(id);
}

WorldActor* NeoWorld::GetActor(EntityID id) {
    auto it = m_Actors.find(id);
    return it != m_Actors.end() ? &it->second : nullptr;
}

void NeoWorld::SetActorTransform(EntityID id, float px, float py, float pz,
                                  float rx, float ry, float rz,
                                  float sx, float sy, float sz) {
    auto it = m_Actors.find(id);
    if (it != m_Actors.end()) {
        auto& a = it->second;
        a.posX = px; a.posY = py; a.posZ = pz;
        a.rotX = rx; a.rotY = ry; a.rotZ = rz;
        a.scaleX = sx; a.scaleY = sy; a.scaleZ = sz;
    }
}

void NeoWorld::Clear() {
    m_Actors.clear();
}

void NeoWorld::Update(float deltaTime) {
    // Update world logic (physics, AI ticks, etc.)
    // For now, integrate with world streaming
}

std::string NeoWorld::ToJSON() const {
    std::ostringstream ss;
    ss << "{\"actors\":[";
    bool first = true;
    for (const auto& [id, a] : m_Actors) {
        if (!first) ss << ",";
        ss << "{\"id\":" << a.id
           << ",\"name\":\"" << a.name
           << "\",\"type\":\"" << a.type
           << "\",\"pos\":[" << a.posX << "," << a.posY << "," << a.posZ
           << "],\"rot\":[" << a.rotX << "," << a.rotY << "," << a.rotZ
           << "],\"scale\":[" << a.scaleX << "," << a.scaleY << "," << a.scaleZ
           << "]}";
        first = false;
    }
    ss << "],\"count\":" << m_Actors.size() << "}";
    return ss.str();
}

} // namespace NeoEngine
