import subprocess
import os

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

print("========================================================================================================================")
print(f"{'FILE PATH':<45} | {'DATE & TIME':<20} | {'AUTHOR / PUSHER':<18} | {'MERGE / MAIN STATUS':<30}")
print("========================================================================================================================")

for file_path in stubs_to_check:
    if not os.path.exists(file_path):
        continue
        
    # 1. Dapatkan Commit Hash, Author, Date ISO, dan Subject Commit Pertama
    cmd_first = ["git", "log", "--follow", "--diff-filter=A", "--format=%H|%an|%ad|%s", "--date=iso", "--", file_path]
    res_first = subprocess.run(cmd_first, capture_output=True, text=True)
    
    if res_first.stdout.strip():
        first_commit_line = res_first.stdout.strip().split('\n')[-1]
        commit_hash, author, date_str, subject = first_commit_line.split('|')
        
        # 2. Cek apakah commit ini merupakan bagian dari Merge Commit
        cmd_parents = ["git", "rev-list", "--parents", "-n", "1", commit_hash]
        res_parents = subprocess.run(cmd_parents, capture_output=True, text=True)
        parents = res_parents.stdout.strip().split()
        
        is_merge_commit = "Direct Commit"
        if len(parents) > 2:
            is_merge_commit = f"Merge Commit ({parents[1][:7]}<-{parents[2][:7]})"
            
        # 3. Cek cabang/branch asal mana yang membawa commit ini
        cmd_branch = ["git", "branch", "-a", "--contains", commit_hash]
        res_branch = subprocess.run(cmd_branch, capture_output=True, text=True)
        branches = [b.strip().replace("* ", "") for b in res_branch.stdout.strip().split('\n') if b.strip()]
        branch_info = ", ".join(branches[:2]) if branches else "No Branch Info"

        print(f"{os.path.basename(file_path):<45} | {date_str[:19]:<20} | {author:<18} | {is_merge_commit} [{branch_info}]")

