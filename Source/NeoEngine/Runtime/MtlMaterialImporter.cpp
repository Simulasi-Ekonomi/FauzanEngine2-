#include "MtlMaterialImporter.h"

#include "ThirdParty/tiny_obj_loader.h"

#include <cmath>

namespace NeoEngine {
namespace {
bool ValidName(std::string_view name){if(name.empty()||name.size()>96U)return false;for(const unsigned char value:name)if(!std::isalnum(value)&&value!='-'&&value!='_'&&value!='.')return false;return true;}
bool Unit(float value){return std::isfinite(value)&&value>=0.0F&&value<=1.0F;}
uint8_t Channel(float value){return static_cast<uint8_t>(std::lround(value*255.0F));}
}
bool MtlMaterialImporter::Import(std::string_view source,std::string_view materialName,MeshMaterial& material){
    if(source.empty()){lastError_=MtlMaterialImportError::EmptySource;return false;}
    if(source.size()>kMaxSourceBytes){lastError_=MtlMaterialImportError::SourceTooLarge;return false;}
    if(!ValidName(materialName)){lastError_=MtlMaterialImportError::InvalidName;return false;}
    tinyobj::ObjReader reader;tinyobj::ObjReaderConfig config{};
    if(!reader.ParseFromString("mtllib in_memory.mtl\n",std::string(source),config)){lastError_=MtlMaterialImportError::ParseFailed;return false;}
    const tinyobj::material_t* selected=nullptr;for(const tinyobj::material_t& candidate:reader.GetMaterials()){if(candidate.name==materialName){if(selected!=nullptr){lastError_=MtlMaterialImportError::DuplicateMaterial;return false;}selected=&candidate;}}
    if(selected==nullptr){lastError_=MtlMaterialImportError::MissingMaterial;return false;}
    const float red=static_cast<float>(selected->diffuse[0]),green=static_cast<float>(selected->diffuse[1]),blue=static_cast<float>(selected->diffuse[2]),alpha=static_cast<float>(selected->dissolve);
    if(!Unit(red)||!Unit(green)||!Unit(blue)||!Unit(alpha)){lastError_=MtlMaterialImportError::InvalidValue;return false;}
    MeshMaterial pending=material;pending.rgba=(static_cast<uint32_t>(Channel(alpha))<<24U)|(static_cast<uint32_t>(Channel(red))<<16U)|(static_cast<uint32_t>(Channel(green))<<8U)|static_cast<uint32_t>(Channel(blue));material=pending;lastError_=MtlMaterialImportError::None;return true;
}
} // namespace NeoEngine
