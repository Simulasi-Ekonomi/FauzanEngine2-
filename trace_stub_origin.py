import subprocess
import os

def get_git_info(file_path):
    try:
        # Ambil commit hash, author, date, dan commit message pertama kali file dibuat
        cmd = ["git", "log", "--follow", "--diff-filter=A", "--format=%H|%an|%ad|%s", "--", file_path]
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.stdout.strip():
            return res.stdout.strip().split('\n')[-1]
        
        # Jika file belum di-commit/untracked
        cmd_untracked = ["git", "status", "--porcelain", file_path]
        res_u = subprocess.run(cmd_untracked, capture_output=True, text=True)
        if res_u.stdout.strip():
            return "UNTRACKED|Local Workspace|Not Committed|File baru belum di-commit"
            
        return "UNKNOWN|Unknown|Unknown|No git history"
    except Exception as e:
        return f"ERROR|{str(e)}||"

stubs_to_check = [
    # Cluster Rendering/Rendering
    "Source/NeoEngine/Rendering/Rendering/RenderGraph/CommandBuffer.h",
    "Source/NeoEngine/Rendering/Rendering/RHI/RHI.cpp",
    "Source/NeoEngine/Rendering/Rendering/RHI/RHI.h",
    "Source/NeoEngine/Rendering/Rendering/Renderer/Renderer.cpp",
    "Source/NeoEngine/Rendering/Rendering/Renderer/Renderer.h",
    "Source/NeoEngine/Rendering/Rendering/RHI/Vulkan/VulkanRHI.cpp",
    "Source/NeoEngine/Rendering/Rendering/RHI/Vulkan/VulkanRHI.h",
    # Cluster Root ECS Stub
    "Source/NeoEngine/ECS/Component.h",
    "Source/NeoEngine/ECS/ComponentStorage.h",
    "Source/NeoEngine/ECS/ECSCore.h",
    "Source/NeoEngine/ECS/Entity.h",
    "Source/NeoEngine/ECS/EntityManager.cpp",
    "Source/NeoEngine/ECS/EntityManager.h",
    "Source/NeoEngine/ECS/Systems/MovementSystem.cpp",
    "Source/NeoEngine/ECS/System.h",
    "Source/NeoEngine/ECS/SystemManager.cpp",
    "Source/NeoEngine/ECS/SystemManager.h",
    # Cluster Core/Log
    "Source/NeoEngine/Core/Log/Logger.cpp",
    "Source/NeoEngine/Core/Log/Logger.h"
]

print("====================================================================================================")
print(f"{'FILE PATH':<60} | {'AUTHOR/SOURCE':<20} | {'COMMIT HASH / STATUS'}")
print("====================================================================================================")

for stub in stubs_to_check:
    if os.path.exists(stub):
        info = get_git_info(stub)
        parts = info.split('|')
        author = parts[1] if len(parts) > 1 else "N/A"
        commit_status = parts[0][:10] if len(parts) > 0 else "N/A"
        print(f"{stub:<60} | {author:<20} | {commit_status}")

