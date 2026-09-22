#include "GLTFMeshBuilder.h"
#include "GLTFJson.h"
#include <rapidjson/document.h>
#include <fstream>
#include <cstring>

namespace NeoEngine {
MeshData GLTFMeshBuilder::Load(const std::string& path){
    MeshData out{};
    std::ifstream f(path); if(!f) return out;
    std::string json((std::istreambuf_iterator<char>(f)),{});
    auto meshes=GLTFMeshBuilder{}.BuildMeshes(json);
    if(!meshes.empty()) return meshes.front().meshData;
    return out;
}
std::vector<GLTFMesh> GLTFMeshBuilder::BuildMeshes(const std::string& json){
    std::vector<GLTFMesh> result; rapidjson::Document doc; doc.Parse(json.c_str(),json.size());
    if(doc.HasParseError()||!doc.IsObject()||!doc.HasMember("meshes")||!doc["meshes"].IsArray()) return result;
    for(const auto& meshValue:doc["meshes"].GetArray()){
        GLTFMesh mesh{};
        if(meshValue.IsObject()&&meshValue.HasMember("primitives")&&meshValue["primitives"].IsArray()){
            for(const auto& prim:meshValue["primitives"].GetArray()){
                if(prim.HasMember("indices")&&prim["indices"].IsUint()) mesh.meshData.indices.push_back(prim["indices"].GetUint());
            }
        }
        result.push_back(std::move(mesh));
    }
    return result;
}
}
