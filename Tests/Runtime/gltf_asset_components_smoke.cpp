#include "Asset/GLTF/GLTFAccessor.h"
#include "Asset/GLTF/GLTFParser.h"
#include "Asset/GLTF/GLTFTextureLoader.h"

#include <array>
#include <cstdio>
#include <fstream>
#include <string>

int main() {
    const std::array<unsigned char, 12> bytes{
        0, 0, 128, 63,
        0, 0, 0, 64,
        0, 0, 64, 64
    };
    const auto values = NeoEngine::GLTFAccessor::ReadFloatArray(bytes.data(), 0, 3, 4);
    if (values.size() != 3U || values[0] != 1.0F || values[1] != 2.0F || values[2] != 3.0F) return 1;
    if (!NeoEngine::GLTFAccessor::ReadFloatArray(nullptr, 0, 1, 4).empty()) return 2;
    if (!NeoEngine::GLTFAccessor::ReadFloatArray(bytes.data(), 0, 1, 2).empty()) return 3;

    GLTFParser parser;
    if (!parser.Parse(R"json({"asset":{"version":"2.0"},"meshes":[{"name":"Cube"},{}],"materials":[{"name":"Default"},{}]})json")) return 4;
    if (parser.GetMeshes().size() != 2U || parser.GetMeshes()[0] != "Cube" || parser.GetMeshes()[1] != "mesh") return 5;
    if (parser.GetMaterials().size() != 2U || parser.GetMaterials()[0] != "Default" || parser.GetMaterials()[1] != "material") return 6;
    if (parser.Parse("{invalid")) return 7;
    if (!parser.GetMeshes().empty() || !parser.GetMaterials().empty()) return 8;

    const std::string path = "/tmp/neo_gltf_texture_loader_smoke.ppm";
    {
        std::ofstream file(path, std::ios::binary);
        file << "P6\n2 1\n255\n";
        const char rgb[] = {static_cast<char>(255), 0, 0, 0, static_cast<char>(255), 0};
        file.write(rgb, sizeof(rgb));
    }
    const auto texture = NeoEngine::GLTFTextureLoader::Load(path);
    if (texture.width != 2U || texture.height != 1U || texture.pixels.size() != 8U) return 9;
    if (texture.pixels[0] != 255U || texture.pixels[3] != 255U) return 10;
    const auto unsupported = NeoEngine::GLTFTextureLoader::Load("/tmp/unsupported.png");
    if (unsupported.width != 0U || unsupported.height != 0U || !unsupported.pixels.empty()) return 11;

    std::remove(path.c_str());
    std::puts("GLTF_ASSET_COMPONENTS_SMOKE_OK accessor=1 parser=1 texture=1 validation=1");
    return 0;
}
