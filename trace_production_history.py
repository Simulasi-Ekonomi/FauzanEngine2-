import subprocess
import os

print("========================================================================================================================")
print(f"{'FILE PATH':<45} | {'LAST COMMITTER':<18} | {'TYPE':<15} | {'PARENT COMMITS / MERGE SOURCE':<35}")
print("========================================================================================================================")

# Ambil seluruh file C++/Header di Source/NeoEngine
cmd_files = ["find", "Source/NeoEngine", "-type", "f", "(", "-name", "*.cpp", "-o", "-name", "*.h", ")"]
res_files = subprocess.run(cmd_files, capture_output=True, text=True)
all_files = res_files.stdout.strip().split('\n')

stubs = [
    "CommandBuffer.h", "RHI.cpp", "RHI.h", "Renderer.cpp", "Renderer.h", "VulkanRHI.cpp", "VulkanRHI.h",
    "Component.h", "ComponentStorage.h", "ECSCore.h", "Entity.h", "EntityManager.cpp", "EntityManager.h",
    "MovementSystem.cpp", "System.h", "SystemManager.cpp", "SystemManager.h", "Logger.cpp", "Logger.h"
]

prod_files = [f for f in all_files if os.path.basename(f) not in stubs and f.strip() != ""]

for file_path in prod_files:
    # 1. Cek commit terakhir untuk file produksi ini
    cmd_log = ["git", "log", "-n", "1", "--format=%H|%an|%ad|%s", "--date=iso", "--", file_path]
    res_log = subprocess.run(cmd_log, capture_output=True, text=True)
    
    if not res_log.stdout.strip():
        continue
        
    commit_hash, author, date_str, subject = res_log.stdout.strip().split('|')
    
    # 2. Cek apakah commit ini merupakan Merge Commit
    cmd_parents = ["git", "rev-list", "--parents", "-n", "1", commit_hash]
    res_parents = subprocess.run(cmd_parents, capture_output=True, text=True)
    parents = res_parents.stdout.strip().split()
    
    if len(parents) > 2:
        commit_type = "Merge Commit"
        parents_info = f"Parents: {parents[1][:7]} + {parents[2][:7]}"
    else:
        commit_type = "Direct Commit"
        parents_info = f"Single Parent: {parents[1][:7]}" if len(parents) > 1 else "Root Commit"
        
    print(f"{os.path.basename(file_path):<45} | {author:<18} | {commit_type:<15} | {parents_info:<35}")

