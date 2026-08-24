#include "Runtime/MtlMaterialImporter.h"

#include <cstdio>
#include <string>

int main(){
    using namespace NeoEngine;
    constexpr const char* valid="newmtl grass\nKd 0.2 0.4 0.6\nd 0.5\n";
    MtlMaterialImporter importer;MeshMaterial material{};if(!importer.Import(valid,"grass",material)||material.rgba!=0x80336699U)return 1;
    const MeshMaterial preserved=material;
    if(importer.Import(valid,"missing",material)||importer.LastError()!=MtlMaterialImportError::MissingMaterial||material.rgba!=preserved.rgba)return 1;
    if(importer.Import("newmtl grass\nKd 0.1 0.2 0.3\nnewmtl grass\nKd 0.3 0.2 0.1\n","grass",material)||importer.LastError()!=MtlMaterialImportError::DuplicateMaterial||material.rgba!=preserved.rgba)return 1;
    if(importer.Import("newmtl grass\nKd 1.1 0.2 0.3\n","grass",material)||importer.LastError()!=MtlMaterialImportError::InvalidValue||material.rgba!=preserved.rgba)return 1;
    if(importer.Import(std::string(MtlMaterialImporter::kMaxSourceBytes+1U,' '),"grass",material)||importer.LastError()!=MtlMaterialImportError::SourceTooLarge||material.rgba!=preserved.rgba)return 1;
    std::printf("MTL_MATERIAL_IMPORTER_SMOKE_OK diffuse=1 alpha=1 atomic=1 rgba=%08X\n",preserved.rgba);
}
