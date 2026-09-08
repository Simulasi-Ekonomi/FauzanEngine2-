#!/bin/bash

# List 19 file stub redundan
stubs=(
    "Source/NeoEngine/Rendering/Rendering/RenderGraph/CommandBuffer.h"
    "Source/NeoEngine/Rendering/Rendering/RHI/RHI.cpp"
    "Source/NeoEngine/Rendering/Rendering/RHI/RHI.h"
    "Source/NeoEngine/Rendering/Rendering/Renderer/Renderer.cpp"
    "Source/NeoEngine/Rendering/Rendering/Renderer/Renderer.h"
    "Source/NeoEngine/Rendering/Rendering/RHI/Vulkan/VulkanRHI.cpp"
    "Source/NeoEngine/Rendering/Rendering/RHI/Vulkan/VulkanRHI.h"
    "Source/NeoEngine/ECS/Component.h"
    "Source/NeoEngine/ECS/ComponentStorage.h"
    "Source/NeoEngine/ECS/ECSCore.h"
    "Source/NeoEngine/ECS/Entity.h"
    "Source/NeoEngine/ECS/EntityManager.cpp"
    "Source/NeoEngine/ECS/EntityManager.h"
    "Source/NeoEngine/ECS/Systems/MovementSystem.cpp"
    "Source/NeoEngine/ECS/System.h"
    "Source/NeoEngine/ECS/SystemManager.cpp"
    "Source/NeoEngine/ECS/SystemManager.h"
    "Source/NeoEngine/Core/Log/Logger.cpp"
    "Source/NeoEngine/Core/Log/Logger.h"
)

echo "=== Eksekusi Penghapusan File Stub ==="
deleted_count=0

for f in "${stubs[@]}"; do
    if [ -f "$f" ]; then
        rm "$f"
        echo "[DELETED] $f"
        ((deleted_count++))
    else
        echo "[NOT FOUND / ALREADY REMOVED] $f"
    fi
done

# Bersihkan direktori kosong bekas stub
rm -rf Source/NeoEngine/Rendering/Rendering
rm -rf Source/NeoEngine/Core/Log
rm -rf Source/NeoEngine/ECS/Systems

echo ""
echo "Total file stub dihapus: $deleted_count / 19"
echo "=== Eksekusi Selesai ==="
