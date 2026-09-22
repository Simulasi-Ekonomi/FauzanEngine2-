#include "GLTFMeshBuilder.h"
#include <rapidjson/document.h>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <stdexcept>

namespace NeoEngine {
namespace {
bool ReadArray(const rapidjson::Value& v, std::vector<float>& out) {
    if (!v.IsArray()) return false;
    out.clear(); out.reserve(v.Size());
    for (auto& x : v.GetArray()) { if (!x.IsNumber()) return false; out.push_back(x.GetFloat()); }
    return true;
}
}
MeshData GLTFMeshBuilder::Load(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) throw std::runtime_error("GLTFMeshBuilder: cannot open file");
    const std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return BuildMeshes(json).empty() ? MeshData{} : BuildMeshes(json).front().meshData;
}
std::vector<GLTFMesh> GLTFMeshBuilder::BuildMeshes(const std::string& json) {
    std::vector<GLTFMesh> result;
    rapidjson::Document doc; doc.Parse(json.c_str());
    if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("meshes") || !doc["meshes"].IsArray()) return result;
    for (const auto& m : doc["meshes"].GetArray()) {
        if (!m.IsObject() || !m.HasMember("primitives") || !m["primitives"].IsArray()) continue;
        for (const auto& p : m["primitives"].GetArray()) {
            if (!p.IsObject() || !p.HasMember("attributes")) continue;
            const auto& a=p["attributes"]; if (!a.IsObject() || !a.HasMember("POSITION")) continue;
            GLTFMesh out; std::vector<float> pos,nrm,uv,idx;
            if (a["POSITION"].IsArray()) ReadArray(a["POSITION"],pos);
            if (a.HasMember("NORMAL") && a["NORMAL"].IsArray()) ReadArray(a["NORMAL"],nrm);
            if (a.HasMember("TEXCOORD_0") && a["TEXCOORD_0"].IsArray()) ReadArray(a["TEXCOORD_0"],uv);
            if (p.HasMember("indices") && p["indices"].IsArray()) ReadArray(p["indices"],idx);
            const size_t count=pos.size()/3; if (count==0 || pos.size()%3) continue;
            out.meshData.vertices.resize(count);
            for(size_t i=0;i<count;++i){std::memcpy(out.meshData.vertices[i].position,&pos[i*3],3*sizeof(float)); if(nrm.size()>=i*3+3) std::memcpy(out.meshData.vertices[i].normal,&nrm[i*3],3*sizeof(float)); if(uv.size()>=i*2+2) std::memcpy(out.meshData.vertices[i].uv,&uv[i*2],2*sizeof(float));}
            if(idx.empty()){out.meshData.indices.resize(count); for(size_t i=0;i<count;++i) out.meshData.indices[i]=static_cast<unsigned int>(i);}
            else {out.meshData.indices.reserve(idx.size()); for(float x:idx){if(x<0) {out.meshData.indices.clear();break;} out.meshData.indices.push_back(static_cast<unsigned int>(x));}}
            result.push_back(std::move(out));
        }
    }
    return result;
}
}