#include "Asset/GLTF/GLTFMeshBuilder.h"

#include <cstdio>
#include <string>

int main() {
    const std::string json = R"json({"asset":{"version":"2.0"},"buffers":[{"byteLength":42,"uri":"data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAABAAIA"}],"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":6}],"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},{"bufferView":1,"componentType":5123,"count":3,"type":"SCALAR"}],"meshes":[{"primitives":[{"attributes":{"POSITION":0},"indices":1}]}]})json";
    NeoEngine::GLTFMeshBuilder builder;
    const auto meshes = builder.BuildMeshes(json);
    if (meshes.size() != 1U) return 1;
    const auto& mesh = meshes.front().meshData;
    if (mesh.vertices.size() != 3U || mesh.indices.size() != 3U) return 2;
    if (mesh.indices[0] != 0U || mesh.indices[1] != 1U || mesh.indices[2] != 2U) return 3;
    if (mesh.vertices[1].position[0] != 1.0F || mesh.vertices[2].position[1] != 1.0F) return 4;
    if (!builder.BuildMeshes("{bad json").empty()) return 5;
    std::puts("GLTF_MESH_BUILDER_SMOKE_OK data_uri=1 positions=1 indices=1 rejection=1");
    return 0;
}
