cmake_minimum_required(VERSION 3.28)

project(NeoEngine LANGUAGES CXX)
find_package(Vulkan REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_triangle.vert.spv" "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_triangle.frag.spv" "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_texture.vert.spv" "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_texture.frag.spv"
    COMMAND "${GLSLANG_VALIDATOR}" -V "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_triangle.vert" -o "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_triangle.vert.spv"
    COMMAND "${GLSLANG_VALIDATOR}" -V "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_triangle.frag" -o "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_triangle.frag.spv"
    COMMAND "${GLSLANG_VALIDATOR}" -V "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_texture.vert" -o "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_texture.vert.spv"
    COMMAND "${GLSLANG_VALIDATOR}" -V "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_texture.frag" -o "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_texture.frag.spv"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_triangle.vert" "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_triangle.frag" "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_texture.vert" "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_texture.frag"
    VERBATIM
)
add_custom_target(neo_shaders DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_triangle.vert.spv" "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_triangle.frag.spv" "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_texture.vert.spv" "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_texture.frag.spv")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# PHASE 1: Core Runtime Sources (Jules RHI + SDK)
# ============================================================================
set(XPBD_RUNTIME_SOURCES
    Agents/AgentCommandGateway.cpp
    Agents/AgentAutonomyPolicy.cpp
    Agents/AgentManualRepairProtocol.cpp
    Agents/PromptToolGraph.cpp
    Animation/Bone.cpp
    Animation/Skeleton.cpp
    Animation/Skinning.cpp
    Animation/SkeletalPoseClip.cpp
    Animation/SkeletalPosePlayer.cpp
    Animation/SkeletalAnimationController.cpp
    Animation/SkeletalLocomotionMetadata.cpp
    Core/Math/Mat4_Transform.cpp
    Core/EngineLoop.cpp
    Core/ECS/ArchetypeManager.cpp
    Core/ECS/EntityManager.cpp
    Core/Memory/PoolAllocator.cpp
    Core/Memory/FrameAllocator.cpp
    Physics/V5/XPBDPhysicsSystem.cpp
    Runtime/NeoRuntime.cpp
    Runtime/RuntimeClock.cpp
    Runtime/RuntimeTimeSystem.cpp
    Runtime/RuntimeTimerQueue.cpp
    Runtime/RenderCamera.cpp
    Runtime/SpriteBatch.cpp
    Runtime/SceneSpriteAdapter.cpp
    Runtime/MeshRenderer.cpp
    Runtime/TinyObjLoader.cpp
    Runtime/ObjMeshImporter.cpp
    Runtime/MtlMaterialImporter.cpp
    Runtime/MaterialStaging.cpp
    Runtime/MeshStaging.cpp
    Runtime/BitmapTextRenderer.cpp
    Runtime/UiInputRouter.cpp
    Runtime/UiLayoutResolver.cpp
    Runtime/UiCanvasRenderer.cpp
    Runtime/AnimationTimeline.cpp
    Runtime/AnimationStateMachine.cpp
    Runtime/AnimationLocomotionBridge.cpp
    Runtime/AnimationSpriteTintBinding.cpp
    Runtime/AnimationPlaybackControlBinding.cpp
    Runtime/PhysicsAnimationLocomotionBridge.cpp
    Runtime/FlipbookFrameSelector.cpp
    Runtime/FlipbookPlayback.cpp
    Runtime/FlipbookRenderBridge.cpp
    Runtime/GameplayPhysicsQuery.cpp
    Runtime/GameplayPhysicsBody.cpp
    Runtime/KinematicCollisionPreflight.cpp
    Runtime/GameplayTriggerTracker.cpp
    Runtime/ScenePhysicsPoseSync.cpp
    Runtime/TextureImportPipeline.cpp
    Runtime/MeshImportPipeline.cpp
    Runtime/MaterialImportPipeline.cpp
    Runtime/TransformAnimationBinding.cpp
    Runtime/RuntimePersistence.cpp
    Runtime/AtomicSaveFile.cpp
    Runtime/AndroidLifecycleGate.cpp
    Runtime/EventSignalBus.cpp
    Runtime/RendererCapability.cpp
    Runtime/SoftwareRenderer.cpp
    Runtime/SoftwareSurfacePresenter.cpp
    Runtime/AssetRegistry.cpp
    Runtime/AssetManifestSnapshot.cpp
    Runtime/AssetReloadDiagnostics.cpp
    Runtime/AssetRefreshDiagnostics.cpp
    Runtime/AssetRefreshExecutor.cpp
    Runtime/PpmTexture.cpp
    Runtime/BmpTexture.cpp
    Runtime/TextureStaging.cpp
    Runtime/SceneWorld.cpp
    Runtime/ActorComponentWorld.cpp
    Runtime/CharacterPawn.cpp
    Runtime/ReplicationWorld.cpp
    Runtime/AssetResourceManager.cpp
    Runtime/EditorSceneDocument.cpp
    Runtime/EditorSceneDocumentCodec.cpp
    Runtime/EditorScenePrefab.cpp
    Runtime/EditorScenePrefabCodec.cpp
    Runtime/PrefabStaging.cpp
    Runtime/EditorSceneSession.cpp
    Runtime/EditorSceneMeshBinder.cpp
    Runtime/EditorSceneSpriteBinder.cpp
    Runtime/AuthoringCatalogVisualBinder.cpp
    Runtime/LocalAuthoringBridge.cpp
    Runtime/SceneMeshAdapter.cpp
    Runtime/SceneRenderAdapter.cpp
    Runtime/KinematicMotionController.cpp
    Runtime/MovementAuthority.cpp
    Runtime/InputMotionBridge.cpp
    Runtime/GridRouteFollower.cpp
    Runtime/RouteRootMotionAdapter.cpp
    Runtime/InputState.cpp
    Runtime/SdlInputBridge.cpp
    Runtime/AudioMixer.cpp
    Runtime/SdlAudioBridge.cpp
    Runtime/VulkanContext.cpp
    
    # ===== Jules RHI 10 Modules (R1-R3 Complete) =====
    Runtime/VulkanGPUBuffer.cpp
    Runtime/VulkanGPUTexture.cpp
    Runtime/VulkanGraphicsPipeline.cpp
    Runtime/VulkanDescriptorManager.cpp
    Runtime/VulkanMeshBufferBuilder.cpp
    Runtime/VulkanRenderPassManager.cpp
    Runtime/VulkanRenderCommandRecorder.cpp
    Runtime/VulkanSwapchainManager.cpp
    Runtime/VulkanSyncPrimitives.cpp
    Runtime/VulkanRenderer3D.cpp
    # ===== End Jules Modules =====
    
    Runtime/VulkanBootstrap.cpp
    Runtime/VulkanShaderProbe.cpp
    Runtime/VulkanOffscreen.cpp
    Runtime/VulkanTextureUpload.cpp
    Runtime/VulkanTexturedOffscreen.cpp
    Runtime/VulkanPresentProbe.cpp
    Runtime/VulkanTexturedPresent.cpp
    Demos/FarmSurfaceDemo.cpp
    Demos/FarmInteractiveSurfaceDemo.cpp
    Demos/MeshSurfaceDemo.cpp
    Demos/AssetImportSurfaceDemo.cpp
    Demos/MaterialImportSurfaceDemo.cpp
    Demos/MaterialRefreshSurfaceDemo.cpp
    Demos/MotionAnimationSurfaceDemo.cpp
    Demos/PhysicsAnimationSurfaceDemo.cpp
    Demos/NpcRouteAnimationSurfaceDemo.cpp
    Demos/TriggerRuntimeSurfaceDemo.cpp
    Demos/TriggerAnimationSurfaceDemo.cpp
    Demos/KinematicPreflightSurfaceDemo.cpp
    Demos/FlipbookSurfaceDemo.cpp
    Demos/HybridSurfaceDemo.cpp
    Runtime/FarmRenderAdapter.cpp
    Runtime/FarmSpriteRenderAdapter.cpp
    Runtime/FarmRenderAssetManifest.cpp
    Runtime/FarmActionPanelController.cpp
    Runtime/FarmPlayerInputBridge.cpp
    Runtime/FarmRuntimeSession.cpp
    Runtime/FarmRuntimeHud.cpp
    Runtime/FarmRuntimeSaveCodec.cpp
    Runtime/AssetRegistry.cpp
    Systems/FarmSystem.cpp
    Systems/CurriculumSystem.cpp
    Systems/AgricultureCurriculum.cpp
    Systems/FarmWorldTool.cpp
    Systems/FarmCanonicalGameTool.cpp
    Systems/FarmAuthoritativeService.cpp
    Systems/FarmAuthoritativeSessionHost.cpp
    Systems/FarmAuthoritativeSessionLoopback.cpp
    Systems/FarmCommerceEntitlementLedger.cpp
    Systems/FarmCommerceCheckpoint.cpp
    Systems/FarmCommerceCheckpointFile.cpp
    Systems/FarmAuthorityCheckpoint.cpp
    Systems/FarmAuthorityCheckpointFile.cpp
    Systems/FarmTelemetryAdapter.cpp
    Systems/AuthoritativeCommandGate.cpp
    Systems/AuthoringCatalog.cpp
    Systems/CommodityCatalog.cpp
    Systems/CommodityEconomyLedger.cpp
    Systems/GameLoanBook.cpp
    Systems/GridNavigation.cpp
    Systems/WorldAuthoring.cpp
    Systems/AuthorityWireProtocol.cpp
    Systems/AuthorityLoopbackServer.cpp
    Systems/TrustSafetySystem.cpp
    Systems/TelemetryOutbox.cpp
    Templates/SudokuGame.cpp
    Templates/TowerDefenseGame.cpp
    Templates/MatchThreeGame.cpp
    Templates/RpgSandboxGame.cpp
    Templates/GameTemplateRegistry.cpp
    Threading/JobSystem.cpp
)

add_library(neo_core SHARED ${XPBD_RUNTIME_SOURCES})
add_dependencies(neo_core neo_shaders)
target_link_libraries(neo_core PRIVATE Vulkan::Vulkan SDL3::SDL3)
target_compile_definitions(neo_core PRIVATE NEO_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Shaders")
target_include_directories(neo_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/ECS
    ${CMAKE_CURRENT_SOURCE_DIR}/Physics
    ${CMAKE_CURRENT_SOURCE_DIR}/Threading
)

# ============================================================================
# Smoke Tests & Executables
# ============================================================================
function(add_xpbd_executable target test_source)
    add_executable(${target} ${test_source} ${XPBD_RUNTIME_SOURCES})
    add_dependencies(${target} neo_shaders)
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/ECS
        ${CMAKE_CURRENT_SOURCE_DIR}/Physics
        ${CMAKE_CURRENT_SOURCE_DIR}/Threading
    )
    target_link_libraries(${target} PRIVATE Vulkan::Vulkan SDL3::SDL3)
    target_compile_definitions(${target} PRIVATE NEO_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Shaders")
endfunction()

add_xpbd_executable(bench_100k ../../Tests/Physics/bench_100k.cpp)

# ===== Jules RHI Smoke Tests (10 modules) =====
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_gpu_buffer_smoke.cpp")
    add_xpbd_executable(vulkan_gpu_buffer_smoke ../../Tests/Runtime/vulkan_gpu_buffer_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_gpu_texture_smoke.cpp")
    add_xpbd_executable(vulkan_gpu_texture_smoke ../../Tests/Runtime/vulkan_gpu_texture_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_graphics_pipeline_smoke.cpp")
    add_xpbd_executable(vulkan_graphics_pipeline_smoke ../../Tests/Runtime/vulkan_graphics_pipeline_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_descriptor_manager_smoke.cpp")
    add_xpbd_executable(vulkan_descriptor_manager_smoke ../../Tests/Runtime/vulkan_descriptor_manager_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_mesh_buffer_builder_smoke.cpp")
    add_xpbd_executable(vulkan_mesh_buffer_builder_smoke ../../Tests/Runtime/vulkan_mesh_buffer_builder_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_render_pass_manager_smoke.cpp")
    add_xpbd_executable(vulkan_render_pass_manager_smoke ../../Tests/Runtime/vulkan_render_pass_manager_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_render_command_recorder_smoke.cpp")
    add_xpbd_executable(vulkan_render_command_recorder_smoke ../../Tests/Runtime/vulkan_render_command_recorder_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_swapchain_manager_smoke.cpp")
    add_xpbd_executable(vulkan_swapchain_manager_smoke ../../Tests/Runtime/vulkan_swapchain_manager_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_sync_primitives_smoke.cpp")
    add_xpbd_executable(vulkan_sync_primitives_smoke ../../Tests/Runtime/vulkan_sync_primitives_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_renderer_3d_smoke.cpp")
    add_xpbd_executable(vulkan_renderer_3d_smoke ../../Tests/Runtime/vulkan_renderer_3d_smoke.cpp)
endif()
# ===== End Jules Tests =====

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Farm/farm_smoke.cpp")
    add_xpbd_executable(farm_smoke ../../Tests/Farm/farm_smoke.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_bootstrap_smoke.cpp")
    add_xpbd_executable(vulkan_bootstrap_smoke ../../Tests/Runtime/vulkan_bootstrap_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_shader_smoke.cpp")
    add_xpbd_executable(vulkan_shader_smoke ../../Tests/Runtime/vulkan_shader_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_offscreen_smoke.cpp")
    add_xpbd_executable(vulkan_offscreen_smoke ../../Tests/Runtime/vulkan_offscreen_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_textured_present_smoke.cpp")
    add_xpbd_executable(vulkan_textured_present_smoke ../../Tests/Runtime/vulkan_textured_present_smoke.cpp)
endif()

message(STATUS "✅ NeoEngine CMake configured: Jules RHI (R1-R3) + Core Runtime")
message(STATUS "   Sources: ${XPBD_RUNTIME_SOURCES}")
