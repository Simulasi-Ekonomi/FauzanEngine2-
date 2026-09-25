#include "Asset/GLTF/GLBLoader.h"
#include "Asset/GLTF/GLTFAnimation.h"
#include "Asset/GLTF/GLTFLoader.h"
#include "Asset/GLTF/GLTFMaterialBuilder.h"
#include "Asset/GLTF/GLTFSkeleton.h"
#include "Animation/SkeletalLocomotionMetadata.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace {
void WriteU32(std::ofstream& out, std::uint32_t value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}
}

int main() {
    NeoEngine::GLTFMaterialBuilder materialBuilder;
    NeoEngine::GLTFMaterial material;
    const std::string materialJson = R"({"materials":[{"name":"Farm","doubleSided":true,"emissiveFactor":[0.1,0.2,0.3],"pbrMetallicRoughness":{"baseColorFactor":[0.2,0.4,0.6,1.0],"metallicFactor":0.7,"roughnessFactor":0.25,"baseColorTexture":{"index":2},"metallicRoughnessTexture":{"index":3}},"normalTexture":{"index":4}}]})";
    assert(materialBuilder.TryBuildFromJSON(materialJson, material));
    assert(material.name == "Farm" && material.doubleSided);
    assert(material.baseColor[2] == 0.6F && material.metallic == 0.7F);
    assert(material.baseColorTexture == "texture_index:2" && material.normalTexture == "texture_index:4");
    assert(!materialBuilder.TryBuildFromJSON(R"({"materials":[{"pbrMetallicRoughness":{"roughnessFactor":2.0}}]})", material));

    GLTFAnimation animation;
    AnimationChannel channel;
    channel.node = 0;
    channel.keyframes = {{0.0F,{0,0,0,1}}, {1.0F,{1,0,0,1}}};
    assert(animation.AddChannelChecked(channel));
    assert(animation.Validate());
    AnimationChannel invalid = channel;
    invalid.keyframes[1].time = -1.0F;
    assert(!animation.AddChannelChecked(invalid));

    Skeleton skeleton;
    Bone root;
    root.parentIndex = -1;
    assert(skeleton.AddBoneChecked(root));
    Bone child;
    child.parentIndex = 0;
    assert(skeleton.AddBoneChecked(child));
    assert(skeleton.Validate());
    Bone invalidBone;
    invalidBone.parentIndex = 4;
    assert(!skeleton.AddBoneChecked(invalidBone));

    NeoEngine::SkeletalPoseClip clip;
    assert(clip.Configure(1));
    NeoEngine::SkeletalPoseKeyframe k0{};
    NeoEngine::SkeletalPoseKeyframe k1{};
    k1.time = 1.0F;
    k1.translation.x = 1.0F;
    assert(clip.SetTrack(0, {k0, k1}));
    NeoEngine::SkeletalLocomotionMetadataError metadataError{};
    assert(NeoEngine::SkeletalLocomotionMetadata::ValidateCardinalOneCell(
        clip, NeoEngine::SkeletalLocomotionDirection::PositiveX, metadataError));
    NeoEngine::SkeletalLocomotionRegistry registry;
    NeoEngine::SkeletalLocomotionClipSlot slot{NeoEngine::SkeletalLocomotionDirection::PositiveX, clip};
    assert(registry.Configure(std::span<const NeoEngine::SkeletalLocomotionClipSlot>(&slot, 1)));
    assert(registry.Find(NeoEngine::SkeletalLocomotionDirection::PositiveX) != nullptr);

    const std::filesystem::path glbPath = std::filesystem::temp_directory_path() / "neoengine_gltf_contract.glb";
    const std::string json = R"({"asset":{"version":"2.0"},"buffers":[]})";
    const std::uint32_t paddedLength = static_cast<std::uint32_t>((json.size() + 3U) & ~3U);
    {
        std::ofstream out(glbPath, std::ios::binary | std::ios::trunc);
        assert(out);
        WriteU32(out, 0x46546C67U);
        WriteU32(out, 2U);
        WriteU32(out, 12U + 8U + paddedLength);
        WriteU32(out, paddedLength);
        WriteU32(out, 0x4E4F534AU);
        out.write(json.data(), static_cast<std::streamsize>(json.size()));
        for (std::uint32_t i = static_cast<std::uint32_t>(json.size()); i < paddedLength; ++i) out.put(' ');
    }
    GLBLoader glb;
    assert(glb.Load(glbPath.string()));
    assert(glb.LastError() == GLBLoadError::None && !glb.GetJSON().empty());
    std::filesystem::remove(glbPath);

    NeoEngine::GLTFLoader loader;
    assert(!loader.ParseMeshesChecked(""));
    assert(!loader.IsValid());

    std::cout << "GLTF_ASSET_CONTRACT_SMOKE_OK material=1 animation=1 skeleton=1 locomotion=1 glb=1 loader_guard=1\n";
    return 0;
}
