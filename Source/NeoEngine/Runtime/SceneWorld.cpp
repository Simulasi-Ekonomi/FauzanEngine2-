#include "SceneWorld.h"

#include <cmath>
#include <cstring>

namespace NeoEngine {
namespace {
template<class T> void Put(std::vector<uint8_t>& bytes,const T& value){const auto* pointer=reinterpret_cast<const uint8_t*>(&value);bytes.insert(bytes.end(),pointer,pointer+sizeof(T));}
template<class T> bool Get(const std::vector<uint8_t>& bytes,size_t& offset,T& value){if(offset+sizeof(T)>bytes.size())return false;std::memcpy(&value,bytes.data()+offset,sizeof(T));offset+=sizeof(T);return true;}
bool ValidTransform(const Transform3& transform){return std::isfinite(transform.x)&&std::isfinite(transform.y)&&std::isfinite(transform.z)&&std::isfinite(transform.rx)&&std::isfinite(transform.ry)&&std::isfinite(transform.rz)&&std::isfinite(transform.sx)&&std::isfinite(transform.sy)&&std::isfinite(transform.sz)&&transform.sx>0.0F&&transform.sy>0.0F&&transform.sz>0.0F;}
Transform3 Compose(const Transform3& parent,const Transform3& local){
    const float lx=local.x*parent.sx,ly=local.y*parent.sy,lz=local.z*parent.sz;
    const float cx=std::cos(parent.rx),sx=std::sin(parent.rx),cy=std::cos(parent.ry),sy=std::sin(parent.ry),cz=std::cos(parent.rz),sz=std::sin(parent.rz);
    const float xRotatedY=lx,yRotatedY=ly*cx-lz*sx,zRotatedY=ly*sx+lz*cx;
    const float xRotatedZ=xRotatedY*cy+zRotatedY*sy,yRotatedZ=yRotatedY,zRotatedZ=-xRotatedY*sy+zRotatedY*cy;
    return {parent.x+xRotatedZ*cz-yRotatedZ*sz,parent.y+xRotatedZ*sz+yRotatedZ*cz,parent.z+zRotatedZ,parent.rx+local.rx,parent.ry+local.ry,parent.rz+local.rz,parent.sx*local.sx,parent.sy*local.sy,parent.sz*local.sz};
}
}
bool SceneWorld::Valid(SceneEntity entity)const{return entity.index<kCapacity&&m_Slots[entity.index].alive&&m_Slots[entity.index].generation==entity.generation;}
void SceneWorld::MarkSubtreeDirty(uint16_t index){if(index>=kCapacity||!m_Slots[index].alive)return;m_Slots[index].worldDirty=true;for(uint16_t child=0;child<kCapacity;++child)if(m_Slots[child].alive&&m_Slots[child].parent==index)MarkSubtreeDirty(child);}
bool SceneWorld::Create(SceneEntity& output){for(uint16_t index=0;index<kCapacity;++index)if(!m_Slots[index].alive){auto& slot=m_Slots[index];slot.alive=true;slot.parent=0xFFFF;slot.local={};slot.world={};slot.worldDirty=true;output={index,slot.generation};++m_AliveCount;return true;}return false;}
bool SceneWorld::Destroy(SceneEntity entity){if(!Valid(entity))return false;auto& slot=m_Slots[entity.index];slot.alive=false;slot.parent=0xFFFF;slot.generation=static_cast<uint16_t>(slot.generation+1U);if(slot.generation==0U)slot.generation=1U;for(uint16_t index=0;index<kCapacity;++index)if(m_Slots[index].alive&&m_Slots[index].parent==entity.index){m_Slots[index].parent=0xFFFF;MarkSubtreeDirty(index);}--m_AliveCount;return UpdateTransforms();}
bool SceneWorld::SetTransform(SceneEntity entity,const Transform3& transform){if(!Valid(entity)){m_LastError=SceneWorldError::InvalidEntity;return false;}if(!ValidTransform(transform)){m_LastError=SceneWorldError::InvalidTransform;return false;}m_Slots[entity.index].local=transform;MarkSubtreeDirty(entity.index);const bool updated=UpdateTransforms();m_LastError=updated?SceneWorldError::None:SceneWorldError::Corrupt;return updated;}
const Transform3* SceneWorld::GetTransform(SceneEntity entity)const{return Valid(entity)?&m_Slots[entity.index].world:nullptr;}
const Transform3* SceneWorld::GetLocalTransform(SceneEntity entity)const{return Valid(entity)?&m_Slots[entity.index].local:nullptr;}
bool SceneWorld::SetParent(SceneEntity child,SceneEntity parent){if(!Valid(child)||!Valid(parent)||child==parent)return false;for(uint16_t index=parent.index;index!=0xFFFF;index=m_Slots[index].parent)if(index==child.index)return false;m_Slots[child.index].parent=parent.index;MarkSubtreeDirty(child.index);return UpdateTransforms();}
bool SceneWorld::ResolveWorld(uint16_t index,std::array<uint8_t,kCapacity>& visiting){Slot& slot=m_Slots[index];if(!slot.worldDirty)return true;if(visiting[index]==1U)return false;visiting[index]=1U;if(slot.parent==0xFFFF)slot.world=slot.local;else{if(slot.parent>=kCapacity||!m_Slots[slot.parent].alive||!ResolveWorld(slot.parent,visiting))return false;slot.world=Compose(m_Slots[slot.parent].world,slot.local);}slot.worldDirty=false;visiting[index]=2U;return true;}
bool SceneWorld::UpdateTransforms(){std::array<uint8_t,kCapacity> visiting{};for(uint16_t index=0;index<kCapacity;++index)if(m_Slots[index].alive&&!ResolveWorld(index,visiting))return false;return true;}
std::vector<uint8_t> SceneWorld::Serialize()const{std::vector<uint8_t> bytes;Put<uint32_t>(bytes,0x31574E53U);Put<uint32_t>(bytes,m_AliveCount);for(uint16_t index=0;index<kCapacity;++index)if(m_Slots[index].alive){Put(bytes,index);Put(bytes,m_Slots[index].generation);Put(bytes,m_Slots[index].parent);Put(bytes,m_Slots[index].local);}return bytes;}
bool SceneWorld::Deserialize(const std::vector<uint8_t>& bytes){size_t offset=0;uint32_t magic=0,count=0;if(!Get(bytes,offset,magic)||!Get(bytes,offset,count)||magic!=0x31574E53U||count>kCapacity){m_LastError=SceneWorldError::Corrupt;return false;}SceneWorld parsed;for(uint32_t entry=0;entry<count;++entry){uint16_t index=0,generation=0,parent=0;Transform3 local{};if(!Get(bytes,offset,index)||!Get(bytes,offset,generation)||!Get(bytes,offset,parent)||!Get(bytes,offset,local)||index>=kCapacity||parsed.m_Slots[index].alive){m_LastError=SceneWorldError::Corrupt;return false;}if(!ValidTransform(local)){m_LastError=SceneWorldError::InvalidTransform;return false;}parsed.m_Slots[index]={generation,parent,true,true,local,{}};++parsed.m_AliveCount;}if(offset!=bytes.size()){m_LastError=SceneWorldError::Corrupt;return false;}for(uint16_t index=0;index<kCapacity;++index)if(parsed.m_Slots[index].alive&&parsed.m_Slots[index].parent!=0xFFFF&&(!parsed.m_Slots[parsed.m_Slots[index].parent].alive||parsed.m_Slots[index].parent==index)){m_LastError=SceneWorldError::Corrupt;return false;}if(!parsed.UpdateTransforms()){m_LastError=SceneWorldError::Corrupt;return false;}*this=parsed;m_LastError=SceneWorldError::None;return true;}
} // namespace NeoEngine
