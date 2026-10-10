#include "GLTFMaterialBuilder.h"

#include <rapidjson/document.h>
#include <cmath>
#include <string>

namespace NeoEngine {
namespace {
bool ReadFloat(const rapidjson::Value& value, const char* key, float& out) {
    if (!value.HasMember(key) || !value[key].IsNumber()) return false;
    out = value[key].GetFloat();
    return std::isfinite(out);
}
bool ReadColor(const rapidjson::Value& value, const char* key, float* out, std::size_t count) {
    if (!value.HasMember(key) || !value[key].IsArray() || value[key].Size() != count) return false;
    for (std::size_t i = 0; i < count; ++i) {
        if (!value[key][static_cast<rapidjson::SizeType>(i)].IsNumber()) return false;
        out[i] = value[key][static_cast<rapidjson::SizeType>(i)].GetFloat();
        if (!std::isfinite(out[i])) return false;
    }
    return true;
}
std::string TextureRef(const rapidjson::Value& textureInfo) {
    if (!textureInfo.IsObject() || !textureInfo.HasMember("index") || !textureInfo["index"].IsUint()) return {};
    return "texture_index:" + std::to_string(textureInfo["index"].GetUint());
}
}
GLTFMaterial GLTFMaterialBuilder::BuildDefaultMaterial() const {
    return {};
}
GLTFMaterial GLTFMaterialBuilder::BuildFromJSON(const std::string& json) const {
    GLTFMaterial result;
    if (!TryBuildFromJSON(json, result)) return BuildDefaultMaterial();
    return result;
}
bool GLTFMaterialBuilder::TryBuildFromJSON(const std::string& json, GLTFMaterial& out) const {
    rapidjson::Document doc;
    doc.Parse(json.data(), json.size());
    if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("materials") || !doc["materials"].IsArray() || doc["materials"].Empty()) return false;
    const auto& material = doc["materials"][0];
    if (!material.IsObject()) return false;
    GLTFMaterial candidate;
    if (material.HasMember("name")) {
        if (!material["name"].IsString()) return false;
        candidate.name = material["name"].GetString();
    }
    if (material.HasMember("doubleSided")) {
        if (!material["doubleSided"].IsBool()) return false;
        candidate.doubleSided = material["doubleSided"].GetBool();
    }
    if (material.HasMember("emissiveFactor") && !ReadColor(material, "emissiveFactor", candidate.emissive, 3U)) return false;
    if (material.HasMember("pbrMetallicRoughness")) {
        const auto& pbr = material["pbrMetallicRoughness"];
        if (!pbr.IsObject()) return false;
        if (pbr.HasMember("baseColorFactor") && !ReadColor(pbr, "baseColorFactor", candidate.baseColor, 4U)) return false;
        if (pbr.HasMember("metallicFactor") && !ReadFloat(pbr, "metallicFactor", candidate.metallic)) return false;
        if (pbr.HasMember("roughnessFactor") && !ReadFloat(pbr, "roughnessFactor", candidate.roughness)) return false;
        if (pbr.HasMember("baseColorTexture")) {
            candidate.baseColorTexture = TextureRef(pbr["baseColorTexture"]);
            if (candidate.baseColorTexture.empty()) return false;
        }
        if (pbr.HasMember("metallicRoughnessTexture")) {
            candidate.metallicRoughnessTexture = TextureRef(pbr["metallicRoughnessTexture"]);
            if (candidate.metallicRoughnessTexture.empty()) return false;
        }
    }
    if (material.HasMember("normalTexture")) {
        candidate.normalTexture = TextureRef(material["normalTexture"]);
        if (candidate.normalTexture.empty()) return false;
    }
    if (candidate.metallic < 0.0F || candidate.metallic > 1.0F || candidate.roughness < 0.0F || candidate.roughness > 1.0F) return false;
    out = std::move(candidate);
    return true;
}
} // namespace NeoEngine