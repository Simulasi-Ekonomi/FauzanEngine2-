#include "Asset/GLTF/GLTFMeshBuilder.h"

#include <cstdio>
#include <string>

int main() {
    const std::string json = R"json({"asset":{"version":"2.0"},"buffers":[{"byteLength":102,"uri":"data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAABAAIAAAAAAAEAAAAAAQAAAACAPwAAAAAAAAAAAAAAAAAAAD8AAAA/AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAA"}],"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":6},{"buffer":0,"byteOffset":42,"byteLength":12},{"buffer":0,"byteOffset":54,"byteLength":48}],"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},{"bufferView":1,"componentType":5123,"count":3,"type":"SCALAR"},{"bufferView":2,"componentType":5121,"count":3,"type":"VEC4"},{"bufferView":3,"componentType":5126,"count":3,"type":"VEC4"}],"meshes":[{"primitives":[{"attributes":{"POSITION":0,"JOINTS_0":2,"WEIGHTS_0":3},"indices":1}]}]})json";
    NeoEngine::GLTFMeshBuilder builder;
    const auto meshes = builder.BuildMeshes(json);
    if (meshes.size() != 1U) return 1;
    const auto& mesh = meshes.front().meshData;
    if (mesh.vertices.size() != 3U || mesh.indices.size() != 3U) return 2;
    if (mesh.indices[0] != 0U || mesh.indices[1] != 1U || mesh.indices[2] != 2U) return 3;
    if (mesh.vertices[1].position[0] != 1.0F || mesh.vertices[2].position[1] != 1.0F) return 4;
    if (mesh.vertices[0].boneIndices[0] != 0U || mesh.vertices[1].boneIndices[0] != 1U || mesh.vertices[2].boneIndices[1] != 1U) return 5;
    if (mesh.vertices[0].boneWeights[0] != 1.0F) return 6;
    if (mesh.vertices[1].boneWeights[0] != 0.5F || mesh.vertices[1].boneWeights[1] != 0.5F) return 7;
    for (const auto& vertex : mesh.vertices) {
        float sum = 0.0F;
        for (float weight : vertex.boneWeights) sum += weight;
        if (sum < 0.99999F || sum > 1.00001F) return 8;
    }
    const std::string partialSkinning = R"json({"asset":{"version":"2.0"},"buffers":[{"byteLength":36,"uri":"data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8"}],"bufferViews":[{"buffer":0,"byteLength":36}],"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"}],"meshes":[{"primitives":[{"attributes":{"POSITION":0,"JOINTS_0":0}}]}]})json";
    if (!builder.BuildMeshes(partialSkinning).empty()) return 9;
    if (!builder.BuildMeshes("{bad json").empty()) return 10;
    std::puts("GLTF_MESH_BUILDER_SMOKE_OK data_uri=1 positions=1 indices=1 skinning=1 normalization=1 malformed_rejection=1");
    return 0;
}
