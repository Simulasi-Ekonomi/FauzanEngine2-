#pragma once
#include <array>
#include <cstdint>
#include <vector>
namespace NeoEngine {
struct SceneEntity{uint16_t index=0xFFFF,generation=0;bool operator==(const SceneEntity&)const=default;};
struct Transform3{float x=0,y=0,z=0,rx=0,ry=0,rz=0,sx=1,sy=1,sz=1;};
enum class SceneWorldError:uint8_t{None,InvalidEntity,InvalidTransform,Corrupt};
class SceneWorld{public:static constexpr uint16_t kCapacity=4096;bool Create(SceneEntity&out);bool Destroy(SceneEntity e);bool SetTransform(SceneEntity e,const Transform3&t);const Transform3*GetTransform(SceneEntity e)const;const Transform3*GetLocalTransform(SceneEntity e)const;bool SetParent(SceneEntity child,SceneEntity parent);bool UpdateTransforms();std::vector<uint8_t>Serialize()const;bool Deserialize(const std::vector<uint8_t>&bytes);uint32_t AliveCount()const{return m_AliveCount;}[[nodiscard]]SceneWorldError LastError()const{return m_LastError;}private:struct Slot{uint16_t generation=1,parent=0xFFFF;bool alive=false,worldDirty=true;Transform3 local{};Transform3 world{};};bool Valid(SceneEntity e)const;void MarkSubtreeDirty(uint16_t index);bool ResolveWorld(uint16_t index,std::array<uint8_t,kCapacity>&visiting);std::array<Slot,kCapacity>m_Slots{};uint32_t m_AliveCount=0;SceneWorldError m_LastError=SceneWorldError::None;};
} 
