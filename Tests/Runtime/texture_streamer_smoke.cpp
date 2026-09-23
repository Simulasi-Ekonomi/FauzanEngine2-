#include "Core/GPU/TextureStreamer.h"
#include "Runtime/VulkanContext.h"

#include <array>
#include <cstdio>
#include <fstream>
#include <string>

int main() {
    NeoEngine::VulkanContext context;
    if (!context.Initialize()) {
        std::puts("TEXTURE_STREAMER_SMOKE_SKIPPED_NO_VULKAN");
        return 0;
    }

    NeoEngine::TextureStreamer streamer;
    if (!streamer.Initialize(context.Device(), context.PhysicalDevice(),
                             context.GraphicsQueue(), context.GraphicsQueueFamily())) return 1;

    const std::array<unsigned char, 16> pixels{
        255, 0, 0, 255,
        0, 255, 0, 255,
        0, 0, 255, 255,
        255, 255, 255, 255
    };
    if (!streamer.StreamIn("memory", 2, 2, pixels.data(), pixels.size())) return 2;
    if (!streamer.Contains("memory") || streamer.Find("memory") == nullptr) return 3;
    if (streamer.Find("memory")->GetImage() == VK_NULL_HANDLE ||
        streamer.Find("memory")->GetImageView() == VK_NULL_HANDLE ||
        streamer.Find("memory")->GetSampler() == VK_NULL_HANDLE) return 4;

    const std::string path = "/tmp/neo_texture_streamer_smoke.ppm";
    {
        std::ofstream file(path, std::ios::binary);
        file << "P6\n2 1\n255\n";
        const char rgb[] = {static_cast<char>(255), 0, 0, 0, static_cast<char>(255), 0};
        file.write(rgb, sizeof(rgb));
    }
    if (!streamer.StreamIn(path)) return 5;
    if (!streamer.Contains(path) || !streamer.Find(path)) return 6;
    if (!streamer.StreamOut("memory") || streamer.Contains("memory")) return 7;
    if (streamer.StreamOut("missing")) return 8;

    streamer.Destroy();
    if (streamer.IsInitialized() || streamer.Find(path) != nullptr) return 9;
    std::remove(path.c_str());

    context.Reset();
    std::puts("TEXTURE_STREAMER_SMOKE_OK init=1 raw_upload=1 ppm_upload=1 eviction=1 destroy=1");
    return 0;
}
