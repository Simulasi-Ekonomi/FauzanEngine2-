#include "GLTFParser.h"
#include <rapidjson/document.h>
bool GLTFParser::Parse(const std::string& json) {
    meshes.clear(); materials.clear();
    rapidjson::Document doc; doc.Parse(json.c_str());
    if(doc.HasParseError() || !doc.IsObject()) return false;
    if(doc.HasMember("meshes") && doc["meshes"].IsArray()) for(const auto& m:doc["meshes"].GetArray()) if(m.IsObject() && m.HasMember("name") && m["name"].IsString()) meshes.emplace_back(m["name"].GetString()); else meshes.emplace_back("mesh");
    if(doc.HasMember("materials") && doc["materials"].IsArray()) for(const auto& m:doc["materials"].GetArray()) if(m.IsObject() && m.HasMember("name") && m["name"].IsString()) materials.emplace_back(m["name"].GetString()); else materials.emplace_back("material");
    return doc.HasMember("asset") && doc["asset"].IsObject();
}
std::vector<std::string> GLTFParser::GetMeshes() const { return meshes; }
std::vector<std::string> GLTFParser::GetMaterials() const { return materials; }