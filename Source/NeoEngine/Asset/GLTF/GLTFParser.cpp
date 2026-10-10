#include "GLTFParser.h"

#include <rapidjson/document.h>

namespace {
bool ReadNamedArray(
    const rapidjson::Value& root,
    const char* key,
    const char* fallback,
    std::vector<std::string>& output)
{
    output.clear();
    if (!root.HasMember(key)) {
        return true;
    }
    const auto& array = root[key];
    if (!array.IsArray()) {
        return false;
    }
    output.reserve(array.Size());
    for (const auto& item : array.GetArray()) {
        if (!item.IsObject()) {
            return false;
        }
        std::string name = fallback;
        if (item.HasMember("name")) {
            if (!item["name"].IsString()) {
                return false;
            }
            name = item["name"].GetString();
            if (name.empty()) {
                name = fallback;
            }
        }
        output.push_back(std::move(name));
    }
    return true;
}
} // namespace

bool GLTFParser::Parse(const std::string& json)
{
    meshes.clear();
    materials.clear();

    rapidjson::Document document;
    document.Parse(json.c_str(), json.size());
    if (document.HasParseError() || !document.IsObject()) {
        return false;
    }

    if (!document.HasMember("asset") || !document["asset"].IsObject() ||
        !document["asset"].HasMember("version") ||
        !document["asset"]["version"].IsString() ||
        std::string(document["asset"]["version"].GetString()) != "2.0") {
        return false;
    }

    if (!ReadNamedArray(document, "meshes", "mesh", meshes) ||
        !ReadNamedArray(document, "materials", "material", materials)) {
        meshes.clear();
        materials.clear();
        return false;
    }
    return true;
}

std::vector<std::string> GLTFParser::GetMeshes() const
{
    return meshes;
}

std::vector<std::string> GLTFParser::GetMaterials() const
{
    return materials;
}
