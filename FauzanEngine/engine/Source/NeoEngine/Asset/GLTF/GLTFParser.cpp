#include "GLTFParser.h"
#include <rapidjson/document.h>

bool GLTFParser::Parse(const std::string& json)
{
    meshes.clear();
    materials.clear();
    rapidjson::Document doc;
    doc.Parse(json.c_str(), json.size());
    if (doc.HasParseError() || !doc.IsObject()) return false;
    if (doc.HasMember("meshes") && doc["meshes"].IsArray()) {
        for (const auto& m : doc["meshes"].GetArray()) {
            if (m.IsObject() && m.HasMember("name") && m["name"].IsString()) meshes.emplace_back(m["name"].GetString());
            else meshes.emplace_back("mesh_" + std::to_string(meshes.size()));
        }
    }
    if (doc.HasMember("materials") && doc["materials"].IsArray()) {
        for (const auto& m : doc["materials"].GetArray()) {
            if (m.IsObject() && m.HasMember("name") && m["name"].IsString()) materials.emplace_back(m["name"].GetString());
            else materials.emplace_back("material_" + std::to_string(materials.size()));
        }
    }
    return true;
}

std::vector<std::string> GLTFParser::GetMeshes() const { return meshes; }
std::vector<std::string> GLTFParser::GetMaterials() const { return materials; }
