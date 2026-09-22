#include "GLTFMeshBuilder.h"

#include <rapidjson/document.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string_view>
#include <vector>

namespace NeoEngine {
namespace {
constexpr std::size_t kMaxBuffers = 256U;
constexpr std::size_t kMaxBufferBytes = 512U * 1024U * 1024U;
constexpr std::size_t kMaxBufferViews = 4096U;
constexpr std::size_t kMaxAccessors = 4096U;
constexpr std::size_t kMaxVerticesPerPrimitive = 1U << 20U;
constexpr std::size_t kMaxIndicesPerPrimitive = 3U * (1U << 20U);

bool ReadFile(const std::filesystem::path& path, std::vector<std::uint8_t>& out) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec) || ec) return false;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size > kMaxBufferBytes) return false;
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return false;
    out.resize(static_cast<std::size_t>(size));
    if (!out.empty()) stream.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(out.size()));
    return static_cast<bool>(stream) || out.empty();
}

int Base64Value(char c) noexcept {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

bool DecodeDataUri(std::string_view uri, std::vector<std::uint8_t>& out) {
    constexpr std::string_view prefix = "data:";
    if (uri.substr(0, prefix.size()) != prefix) return false;
    const std::size_t comma = uri.find(',');
    if (comma == std::string_view::npos || uri.substr(0, comma).find(";base64") == std::string_view::npos) return false;
    const std::string_view encoded = uri.substr(comma + 1U);
    if (encoded.size() > (kMaxBufferBytes * 4U) / 3U + 4U) return false;
    out.clear();
    out.reserve((encoded.size() / 4U) * 3U);
    std::uint32_t accumulator = 0U;
    unsigned bits = 0U;
    for (char c : encoded) {
        if (c == '=') break;
        const int value = Base64Value(c);
        if (value < 0) return false;
        accumulator = (accumulator << 6U) | static_cast<std::uint32_t>(value);
        bits += 6U;
        if (bits >= 8U) {
            bits -= 8U;
            out.push_back(static_cast<std::uint8_t>((accumulator >> bits) & 0xFFU));
            if (out.size() > kMaxBufferBytes) return false;
        }
    }
    return true;
}

std::size_t ComponentSize(int componentType) noexcept {
    switch (componentType) {
        case 5120: case 5121: return 1U;
        case 5122: case 5123: return 2U;
        case 5125: case 5126: return 4U;
        default: return 0U;
    }
}

std::size_t ComponentCount(std::string_view type) noexcept {
    if (type == "SCALAR") return 1U;
    if (type == "VEC2") return 2U;
    if (type == "VEC3") return 3U;
    if (type == "VEC4") return 4U;
    if (type == "MAT2") return 4U;
    if (type == "MAT3") return 9U;
    if (type == "MAT4") return 16U;
    return 0U;
}

bool ReadUnsigned(const std::uint8_t* p, int componentType, std::uint32_t& value) noexcept {
    if (!p) return false;
    switch (componentType) {
        case 5121: value = p[0]; return true;
        case 5123: value = static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8U); return true;
        case 5125: std::memcpy(&value, p, sizeof(value)); return true;
        default: return false;
    }
}

bool ReadFloat(const std::uint8_t* p, int componentType, bool normalized, float& value) noexcept {
    if (!p) return false;
    if (componentType == 5126) { std::memcpy(&value, p, sizeof(value)); return std::isfinite(value); }
    std::uint32_t raw = 0U;
    if (componentType == 5121) raw = p[0];
    else if (componentType == 5123) raw = static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8U);
    else if (componentType == 5125) { std::memcpy(&raw, p, sizeof(raw)); }
    else return false;
    if (!normalized) { value = static_cast<float>(raw); return true; }
    if (componentType == 5121) value = static_cast<float>(raw) / 255.0F;
    else if (componentType == 5123) value = static_cast<float>(raw) / 65535.0F;
    else value = static_cast<float>(raw) / 4294967295.0F;
    return true;
}

struct AccessorView {
    const std::uint8_t* data = nullptr;
    std::size_t stride = 0U;
    std::size_t count = 0U;
    std::size_t components = 0U;
    std::size_t componentSize = 0U;
    int componentType = 0;
    bool normalized = false;
};

bool ResolveAccessor(const rapidjson::Document& doc, const std::vector<std::vector<std::uint8_t>>& buffers,
                     std::size_t accessorIndex, AccessorView& out) {
    if (!doc.HasMember("accessors") || !doc["accessors"].IsArray() || accessorIndex >= doc["accessors"].Size() || accessorIndex >= kMaxAccessors) return false;
    const auto& accessor = doc["accessors"][static_cast<rapidjson::SizeType>(accessorIndex)];
    if (!accessor.IsObject() || !accessor.HasMember("bufferView") || !accessor["bufferView"].IsUint() ||
        !accessor.HasMember("componentType") || !accessor["componentType"].IsInt() || !accessor.HasMember("count") || !accessor["count"].IsUint64() ||
        !accessor.HasMember("type") || !accessor["type"].IsString() || accessor.HasMember("sparse")) return false;
    const std::size_t viewIndex = accessor["bufferView"].GetUint();
    if (!doc.HasMember("bufferViews") || !doc["bufferViews"].IsArray() || viewIndex >= doc["bufferViews"].Size() || viewIndex >= kMaxBufferViews) return false;
    const auto& view = doc["bufferViews"][static_cast<rapidjson::SizeType>(viewIndex)];
    if (!view.IsObject() || !view.HasMember("buffer") || !view["buffer"].IsUint() || !view.HasMember("byteLength") || !view["byteLength"].IsUint64()) return false;
    const std::size_t bufferIndex = view["buffer"].GetUint();
    if (bufferIndex >= buffers.size()) return false;
    const auto& buffer = buffers[bufferIndex];
    const std::size_t viewOffset = view.HasMember("byteOffset") && view["byteOffset"].IsUint64() ? view["byteOffset"].GetUint64() : 0U;
    const std::size_t viewLength = view["byteLength"].GetUint64();
    const std::size_t accessorOffset = accessor.HasMember("byteOffset") && accessor["byteOffset"].IsUint64() ? accessor["byteOffset"].GetUint64() : 0U;
    const std::size_t count = accessor["count"].GetUint64();
    const std::size_t componentSize = ComponentSize(accessor["componentType"].GetInt());
    const std::size_t components = ComponentCount(accessor["type"].GetString());
    if (componentSize == 0U || components == 0U || count == 0U || count > kMaxVerticesPerPrimitive || viewOffset > buffer.size() || viewLength > buffer.size() - viewOffset || accessorOffset > viewLength) return false;
    const std::size_t elementSize = componentSize * components;
    const std::size_t stride = view.HasMember("byteStride") && view["byteStride"].IsUint64() ? view["byteStride"].GetUint64() : elementSize;
    if (stride < elementSize || stride > kMaxBufferBytes || count > 1U && stride > (viewLength - accessorOffset - elementSize) / (count - 1U)) return false;
    const std::size_t start = viewOffset + accessorOffset;
    if (start > buffer.size() || elementSize > buffer.size() - start) return false;
    out = {buffer.data() + start, stride, count, components, componentSize, accessor["componentType"].GetInt(), accessor.HasMember("normalized") && accessor["normalized"].IsBool() && accessor["normalized"].GetBool()};
    return true;
}

bool BuildFromDocument(const rapidjson::Document& doc, const std::vector<std::vector<std::uint8_t>>& buffers, std::vector<GLTFMesh>& result) {
    if (!doc.IsObject() || !doc.HasMember("meshes") || !doc["meshes"].IsArray() || doc["meshes"].Size() > 4096U) return false;
    for (const auto& meshValue : doc["meshes"].GetArray()) {
        if (!meshValue.IsObject() || !meshValue.HasMember("primitives") || !meshValue["primitives"].IsArray()) return false;
        for (const auto& primitive : meshValue["primitives"].GetArray()) {
            if (!primitive.IsObject() || (primitive.HasMember("mode") && (!primitive["mode"].IsUint() || primitive["mode"].GetUint() != 4U)) ||
                !primitive.HasMember("attributes") || !primitive["attributes"].IsObject()) return false;
            const auto& attributes = primitive["attributes"];
            if (!attributes.HasMember("POSITION") || !attributes["POSITION"].IsUint()) return false;
            AccessorView position;
            if (!ResolveAccessor(doc, buffers, attributes["POSITION"].GetUint(), position) || position.components != 3U) return false;
            AccessorView normal, uv;
            const bool hasNormal = attributes.HasMember("NORMAL") && attributes["NORMAL"].IsUint() && ResolveAccessor(doc, buffers, attributes["NORMAL"].GetUint(), normal) && normal.components == 3U && normal.count == position.count;
            const bool hasUv = attributes.HasMember("TEXCOORD_0") && attributes["TEXCOORD_0"].IsUint() && ResolveAccessor(doc, buffers, attributes["TEXCOORD_0"].GetUint(), uv) && uv.components == 2U && uv.count == position.count;
            GLTFMesh mesh{};
            mesh.meshData.vertices.resize(position.count);
            for (std::size_t i = 0U; i < position.count; ++i) {
                const auto* p = position.data + i * position.stride;
                Vertex& vertex = mesh.meshData.vertices[i];
                for (std::size_t component = 0U; component < 3U; ++component) {
                    float value = 0.0F;
                    if (!ReadFloat(p + component * position.componentSize, position.componentType, position.normalized, value) || !std::isfinite(value)) return false;
                    vertex.position[component] = value;
                }
                vertex.normal[0] = 0.0F; vertex.normal[1] = 1.0F; vertex.normal[2] = 0.0F;
                vertex.uv[0] = 0.0F; vertex.uv[1] = 0.0F;
                if (hasNormal) for (std::size_t component = 0U; component < 3U; ++component) {
                    float value = 0.0F; if (!ReadFloat(normal.data + i * normal.stride + component * normal.componentSize, normal.componentType, normal.normalized, value) || !std::isfinite(value)) return false; vertex.normal[component] = value;
                }
                if (hasUv) for (std::size_t component = 0U; component < 2U; ++component) {
                    float value = 0.0F; if (!ReadFloat(uv.data + i * uv.stride + component * uv.componentSize, uv.componentType, uv.normalized, value) || !std::isfinite(value)) return false; vertex.uv[component] = value;
                }
            }
            if (primitive.HasMember("indices")) {
                if (!primitive["indices"].IsUint()) return false;
                AccessorView indices;
                if (!ResolveAccessor(doc, buffers, primitive["indices"].GetUint(), indices) || indices.components != 1U || indices.count > kMaxIndicesPerPrimitive) return false;
                mesh.meshData.indices.resize(indices.count);
                for (std::size_t i = 0U; i < indices.count; ++i) {
                    std::uint32_t index = 0U;
                    if (!ReadUnsigned(indices.data + i * indices.stride, indices.componentType, index) || index >= position.count) return false;
                    mesh.meshData.indices[i] = index;
                }
            } else {
                if (position.count > kMaxIndicesPerPrimitive) return false;
                mesh.meshData.indices.resize(position.count);
                for (std::size_t i = 0U; i < position.count; ++i) mesh.meshData.indices[i] = static_cast<unsigned int>(i);
            }
            if (mesh.meshData.indices.empty() || mesh.meshData.indices.size() % 3U != 0U) return false;
            result.push_back(std::move(mesh));
        }
    }
    return !result.empty();
}

bool ParseDocument(const std::string& json, const std::filesystem::path& basePath, std::vector<GLTFMesh>& result) {
    rapidjson::Document doc;
    doc.Parse(json.data(), json.size());
    if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("buffers") || !doc["buffers"].IsArray() || doc["buffers"].Size() > kMaxBuffers) return false;
    std::vector<std::vector<std::uint8_t>> buffers;
    buffers.reserve(doc["buffers"].Size());
    for (const auto& bufferValue : doc["buffers"].GetArray()) {
        if (!bufferValue.IsObject() || !bufferValue.HasMember("byteLength") || !bufferValue["byteLength"].IsUint64()) return false;
        const std::size_t byteLength = bufferValue["byteLength"].GetUint64();
        if (byteLength > kMaxBufferBytes) return false;
        std::vector<std::uint8_t> data;
        if (bufferValue.HasMember("uri") && bufferValue["uri"].IsString()) {
            const std::string_view uri = bufferValue["uri"].GetString();
            if (uri.substr(0, 5U) == "data:") {
                if (!DecodeDataUri(uri, data)) return false;
            } else {
                if (basePath.empty() || !ReadFile(basePath / std::filesystem::path(uri), data)) return false;
            }
        } else if (bufferValue.HasMember("uri")) return false;
        if (data.size() < byteLength) return false;
        data.resize(byteLength);
        buffers.push_back(std::move(data));
    }
    return BuildFromDocument(doc, buffers, result);
}
} // namespace

MeshData GLTFMeshBuilder::Load(const std::string& path) {
    MeshData out{};
    if (path.empty()) return out;
    std::vector<std::uint8_t> jsonBytes;
    if (!ReadFile(std::filesystem::path(path), jsonBytes) || jsonBytes.empty()) return out;
    std::vector<GLTFMesh> meshes;
    const std::string json(reinterpret_cast<const char*>(jsonBytes.data()), jsonBytes.size());
    if (!ParseDocument(json, std::filesystem::path(path).parent_path(), meshes) || meshes.empty()) return out;
    return std::move(meshes.front().meshData);
}

std::vector<GLTFMesh> GLTFMeshBuilder::BuildMeshes(const std::string& json) {
    std::vector<GLTFMesh> result;
    if (json.empty()) return result;
    (void)ParseDocument(json, {}, result);
    return result;
}

} // namespace NeoEngine
