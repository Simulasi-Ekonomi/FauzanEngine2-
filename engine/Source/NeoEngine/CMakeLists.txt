cmake_minimum_required(VERSION 3.22)
project(NeoEngineCore LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Kumpulkan source secara manual untuk menghindari duplikasi
set(NEO_CORE_SRC
    Core/Core.cpp Core/Engine.cpp Core/EngineLoop.cpp Core/NeoTime.cpp
    Core/Object.cpp Core/SubsystemManager.cpp
    Core/Debug/CoreValidation.cpp Core/Debug/CrashHandler.cpp
    Core/Debug/FrameProfiler.cpp Core/Debug/Log.cpp Core/Debug/Logger.cpp
    Core/Debug/MemoryTracker.cpp Core/Debug/Stats.cpp
    Core/Math/Mat4_Transform.cpp Core/Math/Transform.cpp
    Core/Memory/FrameAllocator.cpp Core/Memory/GarbageCollector.cpp
    Core/Memory/GlobalNewDelete.cpp Core/Memory/LinearAllocator.cpp
    Core/Memory/MemoryAllocator.cpp Core/Memory/MemoryManager.cpp
    Core/Memory/PoolAllocator.cpp
    Core/Object/ObjectBase.cpp Core/Object/ObjectRegistry.cpp
    Core/Physics/PhysicsCore.cpp
    ECS/ECS_Core.cpp ECS/ECSGPUBridge.cpp ECS/EntityManager.cpp
    ECS/SystemManager.cpp ECS/World.cpp ECS/Systems/MovementSystem.cpp
    Platform/Platform.cpp Platform/PlatformTime.cpp
    Platform/Common/Platform.cpp
    Platform/Android/AndroidAppGlue.cpp Platform/Android/AndroidFS.cpp
    Platform/Android/AndroidPlatform.cpp Platform/Android/AndroidTime.cpp
    Platform/Desktop/DesktopPlatform.cpp
    Rendering/RHI/RHI.cpp Rendering/RHI/OpenGL/OpenGLRHI.cpp
    Rendering/RHI/Vulkan/VulkanRHI.cpp Rendering/Renderer/Renderer.cpp
    Rendering/Rendering/FrameGraph/FrameGraph.cpp
    Rendering/Rendering/RenderGraph/RenderGraph.cpp
    Rendering/Rendering/Renderer/Frustum.cpp
    Rendering/Rendering/Renderer/FrustumCulling.cpp
    Rendering/Rendering/Renderer/GPUResourceManager.cpp
    Rendering/Rendering/Renderer/HiZBuffer.cpp
    Rendering/Rendering/Renderer/OcclusionBuffer.cpp
    Rendering/Rendering/Renderer/OcclusionCulling.cpp
    Rendering/Rendering/Renderer/Renderer.cpp
    World/NeoWorld.cpp
    Editor/EditorProvider.cpp
    Memory/NeoMemory.cpp
    AI/AIManager.cpp AI/HermesIntegration.cpp AI/Gemma4Integration.cpp
    AI/RufloIntegration.cpp AI/OpenCodeIntegration.cpp
    AI/BehaviorTree/BehaviorTree.cpp AI/Crowd/CrowdSimulation.cpp
    AI/ML/MLNPCBehavior.cpp AI/Navigation/NavigationMesh.cpp
    Animation/AnimationClip.cpp Animation/AnimationGraph.cpp
    Animation/AnimationPlayer.cpp Animation/Bone.cpp
    Animation/Skeleton.cpp Animation/Skinning.cpp Animation/SkinningGPU.cpp
    Asset/GLTF/GLBLoader.cpp Asset/GLTF/GLTFAccessor.cpp
    Asset/GLTF/GLTFAnimation.cpp Asset/GLTF/GLTFGPUUploader.cpp
    Asset/GLTF/GLTFLoader.cpp Asset/GLTF/GLTFMaterialBuilder.cpp
    Asset/GLTF/GLTFMeshBuilder.cpp Asset/GLTF/GLTFParser.cpp
    Asset/GLTF/GLTFSkeleton.cpp Asset/GLTF/GLTFTextureLoader.cpp
    AssetManagerCore.cpp
    Autonomous/AutonomousManager.cpp
    Physics/BVH.cpp Physics/Collider.cpp Physics/CollisionSAT.cpp
    Physics/ConstraintSolver.cpp Physics/PhysicsEngine.cpp
    Physics/PhysicsWorld.cpp Physics/RigidBody.cpp
    Physics/Destruction/DestructionSystem.cpp
    Physics/Solver/DestructionPhysicsSolver.cpp
    Pipeline/AIGeneration/AIAssetGenerator.cpp
    Networking/Cloud/CloudServerManager.cpp
    Networking/Server/ServerAuthoritativeNetwork.cpp
    Renderer/ClusterRenderer/HierarchicalClusterRenderer.cpp
    Renderer/Culling/GPUOcclusionCulling.cpp
    Renderer/DynamicGI/DynamicGI.cpp Renderer/GI/GlobalIllumination.cpp
    Renderer/GITracing/GITracer.cpp Renderer/GPUDrivenRenderer.cpp
    Renderer/GPUFrustumCulling.cpp Renderer/Lighting/ClusterLighting.cpp
    Renderer/MeshletRenderer.cpp Renderer/Neural/NeuralRenderer.cpp
    Renderer/Particles/GPUParticleSystem.cpp
    Renderer/RayTracing/RayTracingRenderer.cpp
    Renderer/Resources/BindlessResourceManager.cpp
    Renderer/VirtualGeometry/VirtualGeometrySystem.cpp
    Scene/SceneGraph.cpp Scene/SceneNode.cpp Scene/Transform.cpp
    AIControllerCore.cpp AIMassSystem.cpp ActorCore.cpp CameraManagerCore.cpp
    CharacterCore.cpp ECSManager.cpp GameInstanceCore.cpp GameModeCore.cpp
    GameStateCore.cpp HUDCore.cpp InputConfigCore.cpp NeoEditor.cpp
    NeoEditorGizmo.cpp NetworkComponentCore.cpp PlayerControllerCore.cpp
    PlayerStateCore.cpp SaveGameCore.cpp UIManager.cpp UserWidgetCore.cpp
    RHI/NeoRHI.cpp
)

add_library(neo_core SHARED ${NEO_CORE_SRC})

target_include_directories(neo_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Debug
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Log
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Math
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Memory
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Object
    ${CMAKE_CURRENT_SOURCE_DIR}/ECS
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Common
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Android
    ${CMAKE_CURRENT_SOURCE_DIR}/Rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/Rendering/RHI
    ${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/World
    ${CMAKE_CURRENT_SOURCE_DIR}/Editor
    ${CMAKE_CURRENT_SOURCE_DIR}/Memory
    ${CMAKE_CURRENT_SOURCE_DIR}/AI
    ${CMAKE_CURRENT_SOURCE_DIR}/Animation
    ${CMAKE_CURRENT_SOURCE_DIR}/Asset
    ${CMAKE_CURRENT_SOURCE_DIR}/Autonomous
    ${CMAKE_CURRENT_SOURCE_DIR}/Physics
    ${CMAKE_CURRENT_SOURCE_DIR}/Pipeline
    ${CMAKE_CURRENT_SOURCE_DIR}/Networking
    ${CMAKE_CURRENT_SOURCE_DIR}/Renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/Scene
    ${CMAKE_CURRENT_SOURCE_DIR}/Scripting
    ${CMAKE_CURRENT_SOURCE_DIR}/Streaming
)

if(ANDROID)
    find_library(log-lib log)
    if(log-lib)
        target_link_libraries(neo_core ${log-lib})
    endif()
    target_link_libraries(neo_core android log)
endif()

find_package(Vulkan)
if(Vulkan_FOUND)
    target_link_libraries(neo_core Vulkan::Vulkan)
    target_compile_definitions(neo_core PRIVATE NEO_VULKAN_ENABLED=1)
endif()

# =========================================================
# Procedural World Generator Dependencies
# =========================================================
# FastNoiseLite (header-only)
target_include_directories(neo_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/World/FastNoiseLite)
# Cereal (header-only)
target_include_directories(neo_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/World/cereal/include)

# Add ProceduralWorldGenerator source
target_sources(neo_core PRIVATE World/ProceduralWorldGenerator.cpp)
