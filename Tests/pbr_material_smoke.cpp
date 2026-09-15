#include "Runtime/BRDFLut.h"
#include "Runtime/PBRMaterial.h"
#include "Runtime/VulkanContext.h"
#include "Runtime/VulkanDescriptorManager.h"
#include "Runtime/VulkanGPUBuffer.h"
#include "Runtime/VulkanGPUTexture.h"

#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[TEST FAIL] " << msg << " (" << #cond << ")\n"; \
            return 1; \
        } \
    } while (0)

int main() {
    std::cout << "[Smoke Test] Starting pbr_material_smoke...\n";

    const std::string materialPath = "/tmp/neo_pbr_material_smoke.json";
    {
        std::ofstream file(materialPath);
        TEST_CHECK(file.is_open(), "Unable to create temporary material JSON");
        file << R"({
            "name": "SmokeMaterial",
            "parameters": {
                "baseColor": [0.8, 0.6, 0.25, 1.0],
                "metallic": 0.72,
                "roughness": 0.38,
                "normalMapStrength": 0.9,
                "aoStrength": 0.85,
                "flags": 3
            },
            "textures": {
                "baseColor": "textures/test_basecolor.ktx2",
                "normal": "textures/test_normal.ktx2",
                "metallic": "textures/test_metallic.ktx2",
                "roughness": "textures/test_roughness.ktx2",
                "ambientOcclusion": "textures/test_ao.ktx2"
            }
        })";
    }

    NeoEngine::PBRMaterial material;
    TEST_CHECK(material.Load(materialPath), "PBRMaterial JSON load failed");
    TEST_CHECK(material.IsValid(), "PBRMaterial should be valid after load");
    TEST_CHECK(material.GetName() == "SmokeMaterial", "Material name mismatch");
    TEST_CHECK(material.GetParams().baseColor.x > 0.79f, "Base color was not loaded");
    TEST_CHECK(material.GetParams().materialFactors.x > 0.71f, "Metallic was not loaded");
    TEST_CHECK(material.GetParams().materialFactors.y > 0.37f, "Roughness was not loaded");
    TEST_CHECK(material.GetTextureReference(NeoEngine::TextureSlot::Normal).find("normal") != std::string::npos,
               "Normal texture reference was not loaded");

    std::remove(materialPath.c_str());

    NeoEngine::VulkanContext context;
    if (!context.Initialize()) {
        std::cout << "[INFO] Vulkan unavailable; JSON/material validation passed, hardware portion skipped.\n";
        return 0;
    }

    VkDevice device = context.Device();
    VkPhysicalDevice physicalDevice = context.PhysicalDevice();
    TEST_CHECK(device != VK_NULL_HANDLE && physicalDevice != VK_NULL_HANDLE, "Invalid Vulkan device");

    const std::vector<NeoEngine::DescriptorLayoutBindingInfo> bindings = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
        {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
        {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
        {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1}
    };

    NeoEngine::VulkanDescriptorManager descriptorManager;
    TEST_CHECK(descriptorManager.Initialize(device, bindings, 4), "PBR descriptor manager initialization failed");
    TEST_CHECK(material.InitializeDescriptor(descriptorManager), "PBR descriptor set allocation failed");

    NeoEngine::VulkanGPUBuffer parameterBuffer;
    TEST_CHECK(parameterBuffer.Initialize(
                   device, physicalDevice, sizeof(NeoEngine::PBRMaterialParams),
                   NeoEngine::VulkanBufferType::UniformBuffer,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
               "PBR parameter buffer initialization failed");
    TEST_CHECK(material.SetParameterBuffer(parameterBuffer.GetBuffer()), "Parameter buffer binding failed");

    NeoEngine::VulkanGPUTexture texture;
    TEST_CHECK(texture.Initialize(device, physicalDevice, 2, 2, VK_FORMAT_R8G8B8A8_UNORM), "Test texture initialization failed");
    TEST_CHECK(texture.CreateSampler(), "Test texture sampler creation failed");

    for (uint8_t slot = 0; slot < static_cast<uint8_t>(NeoEngine::TextureSlot::Count); ++slot) {
        TEST_CHECK(material.SetTexture(
                       static_cast<NeoEngine::TextureSlot>(slot),
                       texture.GetImageView(), texture.GetSampler()),
                   "PBR texture binding failed");
    }
    TEST_CHECK(material.HasCompleteTextureBindings(), "PBR material texture set is incomplete");
    TEST_CHECK(material.UpdateDescriptor(), "PBR descriptor update failed");
    TEST_CHECK(material.GetDescriptorSet() != VK_NULL_HANDLE, "PBR descriptor set is null");

    NeoEngine::BRDFLut brdf;
    TEST_CHECK(brdf.Initialize(
                   device, physicalDevice, context.GraphicsQueue(), context.GraphicsQueueFamily()),
               "BRDF LUT Vulkan initialization failed");
    TEST_CHECK(brdf.Generate(), "BRDF LUT generation/upload failed");
    TEST_CHECK(brdf.IsValid(), "BRDF LUT should be valid after generation");
    TEST_CHECK(brdf.GetImageView() != VK_NULL_HANDLE, "BRDF LUT image view is null");
    TEST_CHECK(brdf.GetSampler() != VK_NULL_HANDLE, "BRDF LUT sampler is null");

    brdf.Destroy();
    texture.Destroy();
    parameterBuffer.Destroy();
    descriptorManager.Destroy();
    context.Reset();

    std::cout << "[Smoke Test] pbr_material_smoke passed successfully!\n";
    return 0;
}
