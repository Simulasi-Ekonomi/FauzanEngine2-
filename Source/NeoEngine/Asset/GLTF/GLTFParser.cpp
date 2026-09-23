#include "GLTFParser.h"

#include <rapidjson/document.h>

bool GLTFParser::Parse(const std::string& json) {
    meshes.clear();
    materials.clear();

    rapidjson::Document document;
    document.Parse(json.c_str());
    if (document.HasParseError() || !document.IsObject()) {
        return false;
    }

    const auto asset = document.FindMember("asset");
    if (asset == document.MemberEnd() || !asset->value.IsObject()) {
        return false;
    }

    const auto version = asset->value.FindMember("version");
    if (version == asset->value.MemberEnd() || !version->value.IsString() ||
        version->value.GetStringLength() == 0) {
        return false;
    }

    const auto meshesMember = document.FindMember("meshes");
    if (meshesMember != document.MemberEnd() && meshesMember->value.IsArray()) {
        for (const auto& mesh : meshesMember->value.GetArray()) {
            if (mesh.IsObject()) {
                const auto name = mesh.FindMember("name");
                meshes.push_back(name != mesh.MemberEnd() && name->value.IsString()
                    ? name->value.GetString()
                    : "mesh");
            }
        }
    }

    const auto materialsMember = document.FindMember("materials");
    if (materialsMember != document.MemberEnd() && materialsMember->value.IsArray()) {
        for (const auto& material : materialsMember->value.GetArray()) {
            if (material.IsObject()) {
                const auto name = material.FindMember("name");
                materials.push_back(name != material.MemberEnd() && name->value.IsString()
                    ? name->value.GetString()
                    : "material");
            }
        }
    }

    return true;
}

std::vector<std::string> GLTFParser::GetMeshes() const {
    return meshes;
}

std::vector<std::string> GLTFParser::GetMaterials() const {
    return materials;
}
