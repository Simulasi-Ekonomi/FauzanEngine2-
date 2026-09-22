cmake_minimum_required(VERSION 3.28)

project(NeoEngine LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
find_package(Vulkan REQUIRED)
find_package(CURL REQUIRED)
find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
find_program(NEO_GLSLANG_VALIDATOR glslangValidator REQUIRED)
find_path(NEO_RAPIDJSON_INCLUDE_DIR rapidjson/document.h REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NEO_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/neo_shaders")
set(NEO_MESH_VERT_SPV "${NEO_SHADER_OUTPUT_DIR}/neo_mesh.vert.spv")
set(NEO_MESH_FRAG_SPV "${NEO_SHADER_OUTPUT_DIR}/neo_mesh.frag.spv")
set(NEO_TEXTURE_VERT_SPV "${NEO_SHADER_OUTPUT_DIR}/neo_texture.vert.spv")
set(NEO_TEXTURE_FRAG_SPV "${NEO_SHADER_OUTPUT_DIR}/neo_texture.frag.spv")
set(NEO_TRIANGLE_VERT_SPV "${NEO_SHADER_OUTPUT_DIR}/neo_triangle.vert.spv")
set(NEO_TRIANGLE_FRAG_SPV "${NEO_SHADER_OUTPUT_DIR}/neo_triangle.frag.spv")
set(NEO_GPU_INDIRECT_VERT_SPV "${NEO_SHADER_OUTPUT_DIR}/neo_gpu_indirect.vert.spv")

add_custom_command(
    OUTPUT "${NEO_MESH_VERT_SPV}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${NEO_SHADER_OUTPUT_DIR}"
    COMMAND "${NEO_GLSLANG_VALIDATOR}" -V "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_mesh.vert" -o "${NEO_MESH_VERT_SPV}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_mesh.vert"
    VERBATIM
)
add_custom_command(
    OUTPUT "${NEO_MESH_FRAG_SPV}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${NEO_SHADER_OUTPUT_DIR}"
    COMMAND "${NEO_GLSLANG_VALIDATOR}" -V "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_mesh.frag" -o "${NEO_MESH_FRAG_SPV}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_mesh.frag"
    VERBATIM
)
add_custom_command(
    OUTPUT "${NEO_TEXTURE_VERT_SPV}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${NEO_SHADER_OUTPUT_DIR}"
    COMMAND "${NEO_GLSLANG_VALIDATOR}" -V "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_texture.vert" -o "${NEO_TEXTURE_VERT_SPV}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_texture.vert"
    VERBATIM
)
add_custom_command(
    OUTPUT "${NEO_TEXTURE_FRAG_SPV}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${NEO_SHADER_OUTPUT_DIR}"
    COMMAND "${NEO_GLSLANG_VALIDATOR}" -V "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_texture.frag" -o "${NEO_TEXTURE_FRAG_SPV}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_texture.frag"
    VERBATIM
)
add_custom_command(
    OUTPUT "${NEO_TRIANGLE_VERT_SPV}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${NEO_SHADER_OUTPUT_DIR}"
    COMMAND "${NEO_GLSLANG_VALIDATOR}" -V "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_triangle.vert" -o "${NEO_TRIANGLE_VERT_SPV}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_triangle.vert"
    VERBATIM
)
add_custom_command(
    OUTPUT "${NEO_TRIANGLE_FRAG_SPV}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${NEO_SHADER_OUTPUT_DIR}"
    COMMAND "${NEO_GLSLANG_VALIDATOR}" -V "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_triangle.frag" -o "${NEO_TRIANGLE_FRAG_SPV}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_triangle.frag"
    VERBATIM
)
add_custom_command(
    OUTPUT "${NEO_GPU_INDIRECT_VERT_SPV}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${NEO_SHADER_OUTPUT_DIR}"
    COMMAND "${NEO_GLSLANG_VALIDATOR}" -V "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_gpu_indirect.vert" -o "${NEO_GPU_INDIRECT_VERT_SPV}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/neo_gpu_indirect.vert"
    VERBATIM
)
add_custom_target(neoengine_mesh_shaders ALL DEPENDS
    "${NEO_MESH_VERT_SPV}" "${NEO_MESH_FRAG_SPV}"
    "${NEO_TEXTURE_VERT_SPV}" "${NEO_TEXTURE_FRAG_SPV}"
    "${NEO_TRIANGLE_VERT_SPV}" "${NEO_TRIANGLE_FRAG_SPV}"
    "${NEO_GPU_INDIRECT_VERT_SPV}")

set(XPBD_RUNTIME_SOURCES
    Core/ECS/ArchetypeManager.cpp
    Physics/V5/XPBDPhysicsSystem.cpp
    Runtime/AtomicSaveFile.cpp
    Runtime/PBRMaterial.cpp
    Runtime/BRDFLut.cpp
    Runtime/PBRLighting.cpp
    Runtime/PBRIBL.cpp
    Runtime/PBREnvironment.cpp
    Runtime/PBREnvironmentDescriptorSet.cpp
    Runtime/PBRRenderPipeline.cpp
    Runtime/ShaderLibrary.cpp
    Runtime/AssetRegistry.cpp
    Runtime/AssetResourceManager.cpp
    Runtime/AssetStreamingQueue.cpp
    Runtime/GameplayPhysicsQuery.cpp
    Asset/GLTF/GLTFMeshBuilder.cpp
    Asset/GLTF/GLTFGPUUploader.cpp
    Runtime/VulkanAssetUploader.cpp
    Runtime/VulkanContext.cpp
    Runtime/VulkanGPUBuffer.cpp
    Runtime/VulkanDescriptorManager.cpp
    Animation/SkinningGPU.cpp
    Rendering/RHI/Vulkan/VulkanRHI.cpp
    Runtime/VulkanGraphicsPipeline.cpp
    Runtime/VulkanMeshBufferBuilder.cpp
    Runtime/Vulkan3DRenderer.cpp
    Runtime/VulkanPresentProbe.cpp
    Runtime/VulkanTexturedPresent.cpp
    Runtime/NeoRuntime.cpp
    Runtime/NeoRuntimeVulkan3D.cpp
    Runtime/FarmRuntimeSession.cpp
    Systems/FarmTelemetryAdapter.cpp
    Runtime/RuntimeClock.cpp
    Runtime/RuntimePersistence.cpp
    Runtime/SceneWorld.cpp
    Runtime/CanonicalRuntimeWorld.cpp
    Runtime/SceneECSBridge.cpp
    Animation/AnimationClip.cpp
    Animation/AnimationPlayer.cpp
    Animation/AnimationGraph.cpp
    Animation/GPUSkinningPaletteBuffer.cpp
    Runtime/MeshStaging.cpp
    Runtime/MaterialStaging.cpp
    Runtime/TextureStaging.cpp
    Runtime/TinyObjLoader.cpp
    Runtime/ObjMeshImporter.cpp
    Runtime/MtlMaterialImporter.cpp
    Runtime/PpmTexture.cpp
    Runtime/BmpTexture.cpp
    Runtime/EditorScenePrefabCodec.cpp
    Runtime/SceneSpriteAdapter.cpp
    Runtime/RenderCamera.cpp
    Runtime/MeshRenderer.cpp
    Runtime/SpriteBatch.cpp
    Runtime/SoftwareRenderer.cpp
    Runtime/WavAudioParser.cpp
    Runtime/AudioMixer.cpp
    Runtime/AudioComponent.cpp
    Runtime/SdlAudioBridge.cpp
    Runtime/InputState.cpp
    Runtime/SdlInputBridge.cpp
    Runtime/UiInputRouter.cpp
    Runtime/EditorSceneDocumentCodec.cpp
    Runtime/EditorSceneDocument.cpp
    Runtime/EditorSceneMeshBinder.cpp
    Runtime/EditorSceneSpriteBinder.cpp
    Runtime/SceneMeshAdapter.cpp
    Runtime/SceneRenderAdapter.cpp
    Runtime/EditorScenePrefab.cpp
    Runtime/PrefabStaging.cpp
    Runtime/EditorSceneSession.cpp
    Runtime/EditorSceneAgentAPI.cpp
    Runtime/SoftwareSurfacePresenter.cpp
    Runtime/GridRouteFollower.cpp
    Runtime/FarmRuntimeHud.cpp
    Runtime/RuntimeTimeSystem.cpp
    Runtime/EventSignalBus.cpp
    Runtime/FarmRenderAssetManifest.cpp
    Runtime/FarmRenderAdapter.cpp
    Runtime/FarmSpriteRenderAdapter.cpp
    Runtime/ActorComponentWorld.cpp
    Runtime/ReplicationWorld.cpp
    Runtime/KinematicMotionController.cpp
    Runtime/FarmPlayerInputBridge.cpp
    Runtime/RuntimeTimerQueue.cpp
    Runtime/MovementAuthority.cpp
    Runtime/FarmActionPanelController.cpp
    Runtime/UiCanvasRenderer.cpp
    Runtime/BitmapTextRenderer.cpp
    Runtime/UiLayoutResolver.cpp
    Runtime/RouteRootMotionAdapter.cpp
    Runtime/InputMotionBridge.cpp
    Animation/SkeletalAnimationController.cpp
    Animation/SkeletalPoseClip.cpp
    Animation/Bone.cpp
    Core/Math/Mat4_Transform.cpp
    Animation/SkeletalPosePlayer.cpp
    Animation/Skeleton.cpp
    Animation/Skinning.cpp
    Systems/AuthorityLoopbackServer.cpp
    Systems/AuthoritativeCommandGate.cpp
    Systems/AuthorityWireProtocol.cpp
    Systems/FarmAuthoritativeService.cpp
    Systems/FarmAuthoritativeSessionHost.cpp
    Systems/FarmAuthoritativeSessionLoopback.cpp
    Systems/FarmAuthorityCheckpoint.cpp
    Systems/FarmCanonicalGameTool.cpp
    Systems/FarmCommerceEntitlementLedger.cpp
    Systems/FarmCommerceCheckpoint.cpp
    Systems/FarmCommerceCheckpointFile.cpp
    Systems/CommodityCatalog.cpp
    Systems/CommodityEconomyLedger.cpp
    Systems/FarmWorldTool.cpp
    Systems/ItemSerialTracker.cpp
    Systems/TelemetryOutbox.cpp
    Systems/TrustSafetySystem.cpp
    Systems/GridNavigation.cpp
    Systems/WorldAuthoring.cpp
    Systems/CurriculumSystem.cpp
    Systems/AgricultureCurriculum.cpp
    Systems/AuthoringCatalog.cpp
    Systems/FarmSystem.cpp
    Templates/GameTemplateRegistry.cpp
    Templates/MatchThreeGame.cpp
    Templates/RpgSandboxGame.cpp
    Templates/SudokuGame.cpp
    Templates/TowerDefenseGame.cpp
    Threading/JobSystem.cpp
    Renderer/GPUDrivenRenderer.cpp
    Renderer/GPUFrustumCulling.cpp
)

add_library(NeoEngineRuntime STATIC ${XPBD_RUNTIME_SOURCES})
add_dependencies(NeoEngineRuntime neoengine_mesh_shaders)

# Keep canonical ECS/XPBD translation units explicit so smoke executables cannot
# lose their non-template symbols through an incomplete runtime source manifest.
target_sources(NeoEngineRuntime PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/ECS/ArchetypeManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Physics/V5/XPBDPhysicsSystem.cpp"
)
target_include_directories(NeoEngineRuntime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(NeoEngineRuntime PRIVATE ${JSONCPP_INCLUDE_DIRS} ${Vulkan_INCLUDE_DIRS} ${NEO_RAPIDJSON_INCLUDE_DIR})
target_compile_definitions(NeoEngineRuntime PUBLIC
    NEO_SHADER_DIR="${NEO_SHADER_OUTPUT_DIR}"
    GLM_ENABLE_EXPERIMENTAL
)
target_compile_options(NeoEngineRuntime PRIVATE ${JSONCPP_CFLAGS_OTHER})
target_link_libraries(NeoEngineRuntime PUBLIC ${JSONCPP_LIBRARIES} Vulkan::Vulkan SDL3::SDL3 CURL::libcurl)
if(ANDROID)
    target_link_libraries(NeoEngineRuntime PUBLIC log)
endif()

function(add_xpbd_executable target_name source_file)
    add_executable(${target_name} ${source_file})
    target_include_directories(${target_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Runtime
        ${CMAKE_CURRENT_SOURCE_DIR}/Renderer
        ${CMAKE_CURRENT_SOURCE_DIR}/Physics
        ${CMAKE_CURRENT_SOURCE_DIR}/Platform
        ${JSONCPP_INCLUDE_DIRS}
        ${Vulkan_INCLUDE_DIRS}
    )
    target_link_libraries(${target_name} PRIVATE NeoEngineRuntime)
    # Release disables assert(); smoke targets must retain their executable contract checks.
    target_compile_options(${target_name} PRIVATE $<$<CONFIG:Release>:-UNDEBUG>)
endfunction()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/canonical_runtime_world_smoke.cpp")
    add_xpbd_executable(canonical_runtime_world_smoke ../../Tests/Runtime/canonical_runtime_world_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/neo_runtime_ecs_integration_smoke.cpp")
    add_xpbd_executable(neo_runtime_ecs_integration_smoke ../../Tests/Runtime/neo_runtime_ecs_integration_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/runtime_frame_contract_smoke.cpp")
    add_xpbd_executable(runtime_frame_contract_smoke ../../Tests/Runtime/runtime_frame_contract_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/asset_streaming_queue_smoke.cpp")
    add_xpbd_executable(asset_streaming_queue_smoke ../../Tests/Runtime/asset_streaming_queue_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/gameplay_physics_query_smoke.cpp")
    add_xpbd_executable(gameplay_physics_query_smoke ../../Tests/Runtime/gameplay_physics_query_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/gltf_mesh_builder_smoke.cpp")
    add_xpbd_executable(gltf_mesh_builder_smoke ../../Tests/Runtime/gltf_mesh_builder_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/gltf_gpu_uploader_smoke.cpp")
    add_xpbd_executable(gltf_gpu_uploader_smoke ../../Tests/Runtime/gltf_gpu_uploader_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/asset_resource_manager_smoke.cpp")
    add_xpbd_executable(asset_resource_manager_smoke ../../Tests/Runtime/asset_resource_manager_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/farm_vulkan_textured_present_smoke.cpp")
    add_xpbd_executable(farm_vulkan_textured_present_smoke ../../Tests/Runtime/farm_vulkan_textured_present_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/animation_graph_smoke.cpp")
    add_xpbd_executable(animation_graph_smoke ../../Tests/Runtime/animation_graph_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/gpu_skinning_palette_buffer_smoke.cpp")
    add_xpbd_executable(gpu_skinning_palette_buffer_smoke ../../Tests/Runtime/gpu_skinning_palette_buffer_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/skinning_gpu_smoke.cpp")
    add_xpbd_executable(skinning_gpu_smoke ../../Tests/Runtime/skinning_gpu_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_textured_present_smoke.cpp")
    add_xpbd_executable(vulkan_bootstrap_smoke ../../Tests/Runtime/vulkan_bootstrap_smoke.cpp)
    add_xpbd_executable(vulkan_descriptor_manager_smoke ../../Tests/Runtime/vulkan_descriptor_manager_smoke.cpp)
    add_xpbd_executable(vulkan_gpu_buffer_smoke ../../Tests/Runtime/vulkan_gpu_buffer_smoke.cpp)
    add_xpbd_executable(vulkan_gpu_texture_smoke ../../Tests/Runtime/vulkan_gpu_texture_smoke.cpp)
    add_xpbd_executable(vulkan_graphics_pipeline_smoke ../../Tests/Runtime/vulkan_graphics_pipeline_smoke.cpp)
    add_xpbd_executable(vulkan_mesh_buffer_builder_smoke ../../Tests/Runtime/vulkan_mesh_buffer_builder_smoke.cpp)
    add_xpbd_executable(vulkan_offscreen_smoke ../../Tests/Runtime/vulkan_offscreen_smoke.cpp)
    add_xpbd_executable(vulkan_render_command_recorder_smoke ../../Tests/Runtime/vulkan_render_command_recorder_smoke.cpp)
    add_xpbd_executable(vulkan_render_pass_manager_smoke ../../Tests/Runtime/vulkan_render_pass_manager_smoke.cpp)
    add_xpbd_executable(vulkan_renderer_3d_smoke ../../Tests/Runtime/vulkan_renderer_3d_smoke.cpp)
    add_xpbd_executable(vulkan_shader_smoke ../../Tests/Runtime/vulkan_shader_smoke.cpp)
    add_xpbd_executable(vulkan_swapchain_manager_smoke ../../Tests/Runtime/vulkan_swapchain_manager_smoke.cpp)
    add_xpbd_executable(vulkan_sync_primitives_smoke ../../Tests/Runtime/vulkan_sync_primitives_smoke.cpp)
    add_xpbd_executable(vulkan_textured_present_smoke ../../Tests/Runtime/vulkan_textured_present_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/scene_vulkan_render_adapter_smoke.cpp")
    add_xpbd_executable(scene_vulkan_render_adapter_smoke ../../Tests/Runtime/scene_vulkan_render_adapter_smoke.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Tests/r3_gpu_indirect_10k_execution_smoke.cpp")
    add_xpbd_executable(r3_gpu_indirect_10k_execution_smoke Tests/r3_gpu_indirect_10k_execution_smoke.cpp)
    add_xpbd_executable(r3_gpu_indirect_capacity_smoke Tests/r3_gpu_indirect_capacity_smoke.cpp)
    add_xpbd_executable(gpu_driven_renderer_smoke Tests/gpu_driven_renderer_smoke.cpp)
    add_xpbd_executable(vulkan_mesh_indirect_batch_smoke Tests/vulkan_mesh_indirect_batch_smoke.cpp)
    add_xpbd_executable(vulkan_mesh_indirect_batch_gpu_smoke Tests/vulkan_mesh_indirect_batch_gpu_smoke.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/pbr_material_smoke.cpp")
    add_xpbd_executable(pbr_material_smoke ../../Tests/pbr_material_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/pbr_lighting_smoke.cpp")
    add_xpbd_executable(pbr_lighting_smoke ../../Tests/pbr_lighting_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/pbr_ibl_smoke.cpp")
    add_xpbd_executable(pbr_ibl_smoke ../../Tests/pbr_ibl_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/pbr_render_pipeline_smoke.cpp")
    add_xpbd_executable(pbr_render_pipeline_smoke ../../Tests/pbr_render_pipeline_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/pbr_environment_smoke.cpp")
    add_xpbd_executable(pbr_environment_smoke ../../Tests/pbr_environment_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/pbr_environment_descriptor_smoke.cpp")
    add_xpbd_executable(pbr_environment_descriptor_smoke ../../Tests/pbr_environment_descriptor_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/shader_library_smoke.cpp")
    add_xpbd_executable(shader_library_smoke ../../Tests/shader_library_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/sdl_audio_bridge_smoke.cpp")
    add_xpbd_executable(sdl_audio_bridge_smoke ../../Tests/Runtime/sdl_audio_bridge_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/audio_mixer_smoke.cpp")
    add_xpbd_executable(audio_mixer_smoke ../../Tests/Runtime/audio_mixer_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/sdl_input_bridge_smoke.cpp")
    add_xpbd_executable(sdl_input_bridge_smoke ../../Tests/Runtime/sdl_input_bridge_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/input_state_smoke.cpp")
    add_xpbd_executable(input_state_smoke ../../Tests/Runtime/input_state_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/ui_input_router_smoke.cpp")
    add_xpbd_executable(ui_input_router_smoke ../../Tests/Runtime/ui_input_router_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/audio_subsystem_smoke.cpp")
    add_xpbd_executable(audio_subsystem_smoke ../../Tests/Runtime/audio_subsystem_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/editor_subsystem_smoke.cpp")
    add_xpbd_executable(editor_subsystem_smoke ../../Tests/Runtime/editor_subsystem_smoke.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Farm/farm_canonical_game_tool_smoke.cpp")
    add_xpbd_executable(farm_canonical_game_tool_smoke ../../Tests/Farm/farm_canonical_game_tool_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/neo_runtime_farm_vertical_slice_smoke.cpp")
    add_xpbd_executable(neo_runtime_farm_vertical_slice_smoke ../../Tests/Runtime/neo_runtime_farm_vertical_slice_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Farm/farm_authoritative_session_loopback_smoke.cpp")
    add_xpbd_executable(farm_authoritative_session_loopback_smoke ../../Tests/Farm/farm_authoritative_session_loopback_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Systems/authority_loopback_transport_smoke.cpp")
    add_xpbd_executable(authority_loopback_transport_smoke ../../Tests/Systems/authority_loopback_transport_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Farm/farm_commerce_entitlement_smoke.cpp")
    add_xpbd_executable(farm_commerce_entitlement_smoke ../../Tests/Farm/farm_commerce_entitlement_smoke.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Farm/farm_commerce_checkpoint_smoke.cpp")
    add_xpbd_executable(farm_commerce_checkpoint_smoke ../../Tests/Farm/farm_commerce_checkpoint_smoke.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Farm/farm_authoritative_service_smoke.cpp")
    add_xpbd_executable(farm_authoritative_service_smoke ../../Tests/Farm/farm_authoritative_service_smoke.cpp)
endif()

message(STATUS "NeoEngine CMake configured with canonical Vulkan 3D shader generation, Renderer, Audio, Editor, Asset, and Farm runtime sources")

add_xpbd_executable(r6_anti_cheat_100_smoke ../../Tests/r6_anti_cheat_100_smoke.cpp)
add_xpbd_executable(r7_economy_commerce_100_smoke ../../Tests/r7_economy_commerce_100_smoke.cpp)
add_xpbd_executable(r8_persistence_recovery_100_smoke ../../Tests/r8_persistence_recovery_100_smoke.cpp)
add_xpbd_executable(r9_live_ops_telemetry_100_smoke ../../Tests/r9_live_ops_telemetry_100_smoke.cpp)
add_xpbd_executable(r10_security_privacy_100_smoke ../../Tests/r10_security_privacy_100_smoke.cpp)
add_xpbd_executable(comprehensive_engine_verification_smoke ../../Tests/comprehensive_engine_verification_smoke.cpp)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Farm/farm_telemetry_smoke.cpp")
    add_xpbd_executable(farm_telemetry_smoke ../../Tests/Farm/farm_telemetry_smoke.cpp)
endif()
