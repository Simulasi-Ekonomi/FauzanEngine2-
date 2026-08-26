cmake_minimum_required(VERSION 3.28)

project(NeoEngine LANGUAGES CXX)
find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)
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
    Runtime/VulkanBootstrap.cpp
    Runtime/VulkanShaderProbe.cpp
    Runtime/VulkanOffscreen.cpp
    Runtime/VulkanTextureUpload.cpp
    Runtime/VulkanTexturedOffscreen.cpp
    Runtime/VulkanPresentProbe.cpp
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
    Systems/FarmAuthoritativeService.cpp
    Systems/FarmAuthorityCheckpoint.cpp
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
target_link_libraries(neo_core PRIVATE Vulkan::Vulkan SDL2::SDL2)
target_compile_definitions(neo_core PRIVATE NEO_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Shaders")
target_include_directories(neo_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/ECS
    ${CMAKE_CURRENT_SOURCE_DIR}/Physics
    ${CMAKE_CURRENT_SOURCE_DIR}/Threading
)

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
    target_link_libraries(${target} PRIVATE Vulkan::Vulkan SDL2::SDL2)
    target_compile_definitions(${target} PRIVATE NEO_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Shaders")
endfunction()

add_xpbd_executable(bench_100k ../../Tests/Physics/bench_100k.cpp)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Farm/farm_smoke.cpp")
    add_xpbd_executable(farm_smoke ../../Tests/Farm/farm_smoke.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Farm/farm_demo.cpp")
    add_xpbd_executable(farm_demo ../../Tests/Farm/farm_demo.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Farm/farm_telemetry_smoke.cpp")
    add_xpbd_executable(farm_telemetry_smoke ../../Tests/Farm/farm_telemetry_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Farm/farm_trust_smoke.cpp")
    add_xpbd_executable(farm_trust_smoke ../../Tests/Farm/farm_trust_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Farm/farm_world_tool_smoke.cpp")
    add_xpbd_executable(farm_world_tool_smoke ../../Tests/Farm/farm_world_tool_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Farm/farm_scene_sync_smoke.cpp")
    add_xpbd_executable(farm_scene_sync_smoke ../../Tests/Farm/farm_scene_sync_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Farm/farm_authoritative_service_smoke.cpp")
    add_xpbd_executable(farm_authoritative_service_smoke ../../Tests/Farm/farm_authoritative_service_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Farm/farm_authority_loopback_smoke.cpp")
    add_xpbd_executable(farm_authority_loopback_smoke ../../Tests/Farm/farm_authority_loopback_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Farm/farm_authority_checkpoint_smoke.cpp")
    add_xpbd_executable(farm_authority_checkpoint_smoke ../../Tests/Farm/farm_authority_checkpoint_smoke.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Agents/agent_gateway_smoke.cpp")
    add_xpbd_executable(agent_gateway_smoke ../../Tests/Agents/agent_gateway_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Agents/prompt_tool_graph_smoke.cpp")
    add_xpbd_executable(prompt_tool_graph_smoke ../../Tests/Agents/prompt_tool_graph_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Agents/agent_autonomy_policy_smoke.cpp")
    add_xpbd_executable(agent_autonomy_policy_smoke ../../Tests/Agents/agent_autonomy_policy_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Agents/agent_manual_repair_protocol_smoke.cpp")
    add_xpbd_executable(agent_manual_repair_protocol_smoke ../../Tests/Agents/agent_manual_repair_protocol_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Templates/sudoku_smoke.cpp")
    add_xpbd_executable(sudoku_smoke ../../Tests/Templates/sudoku_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Templates/template_registry_smoke.cpp")
    add_xpbd_executable(template_registry_smoke ../../Tests/Templates/template_registry_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Templates/tower_defense_smoke.cpp")
    add_xpbd_executable(tower_defense_smoke ../../Tests/Templates/tower_defense_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Templates/match_three_smoke.cpp")
    add_xpbd_executable(match_three_smoke ../../Tests/Templates/match_three_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Templates/rpg_sandbox_smoke.cpp")
    add_xpbd_executable(rpg_sandbox_smoke ../../Tests/Templates/rpg_sandbox_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/runtime_smoke.cpp")
    add_xpbd_executable(runtime_smoke ../../Tests/Runtime/runtime_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/runtime_clock_smoke.cpp")
    add_xpbd_executable(runtime_clock_smoke ../../Tests/Runtime/runtime_clock_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/runtime_time_system_smoke.cpp")
    add_xpbd_executable(runtime_time_system_smoke ../../Tests/Runtime/runtime_time_system_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/time_curriculum_vertical_slice_smoke.cpp")
    add_xpbd_executable(time_curriculum_vertical_slice_smoke ../../Tests/Runtime/time_curriculum_vertical_slice_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/farm_runtime_session_curriculum_smoke.cpp")
    add_xpbd_executable(farm_runtime_session_curriculum_smoke ../../Tests/Runtime/farm_runtime_session_curriculum_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/runtime_time_integration_smoke.cpp")
    add_xpbd_executable(runtime_time_integration_smoke ../../Tests/Runtime/runtime_time_integration_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/curriculum_validation_smoke.cpp")
    add_xpbd_executable(curriculum_validation_smoke ../../Tests/Runtime/curriculum_validation_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/actor_component_world_smoke.cpp")
    add_xpbd_executable(actor_component_world_smoke ../../Tests/Runtime/actor_component_world_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/character_pawn_smoke.cpp")
    add_xpbd_executable(character_pawn_smoke ../../Tests/Runtime/character_pawn_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/replication_world_smoke.cpp")
    add_xpbd_executable(replication_world_smoke ../../Tests/Runtime/replication_world_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/asset_resource_manager_smoke.cpp")
    add_xpbd_executable(asset_resource_manager_smoke ../../Tests/Runtime/asset_resource_manager_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/runtime_foundation_integration_smoke.cpp")
    add_xpbd_executable(runtime_foundation_integration_smoke ../../Tests/Runtime/runtime_foundation_integration_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/runtime_object_pool_smoke.cpp")
    add_xpbd_executable(runtime_object_pool_smoke ../../Tests/Runtime/runtime_object_pool_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/runtime_timer_queue_smoke.cpp")
    add_xpbd_executable(runtime_timer_queue_smoke ../../Tests/Runtime/runtime_timer_queue_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/runtime_foundations_integration_smoke.cpp")
    add_xpbd_executable(runtime_foundations_integration_smoke ../../Tests/Runtime/runtime_foundations_integration_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Rendering/render_camera_smoke.cpp")
    add_xpbd_executable(render_camera_smoke ../../Tests/Rendering/render_camera_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Rendering/sprite_batch_smoke.cpp")
    add_xpbd_executable(sprite_batch_smoke ../../Tests/Rendering/sprite_batch_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/sprite_atlas_source_rect_smoke.cpp")
    add_xpbd_executable(sprite_atlas_source_rect_smoke ../../Tests/Runtime/sprite_atlas_source_rect_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Rendering/mesh_renderer_smoke.cpp")
    add_xpbd_executable(mesh_renderer_smoke ../../Tests/Rendering/mesh_renderer_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Rendering/obj_mesh_importer_smoke.cpp")
    add_xpbd_executable(obj_mesh_importer_smoke ../../Tests/Rendering/obj_mesh_importer_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Rendering/mtl_material_importer_smoke.cpp")
    add_xpbd_executable(mtl_material_importer_smoke ../../Tests/Rendering/mtl_material_importer_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Rendering/material_staging_smoke.cpp")
    add_xpbd_executable(material_staging_smoke ../../Tests/Rendering/material_staging_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Rendering/mesh_staging_smoke.cpp")
    add_xpbd_executable(mesh_staging_smoke ../../Tests/Rendering/mesh_staging_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Rendering/scene_mesh_adapter_smoke.cpp")
    add_xpbd_executable(scene_mesh_adapter_smoke ../../Tests/Rendering/scene_mesh_adapter_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Rendering/scene_render_adapter_smoke.cpp")
    add_xpbd_executable(scene_render_adapter_smoke ../../Tests/Rendering/scene_render_adapter_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Rendering/bitmap_text_renderer_smoke.cpp")
    add_xpbd_executable(bitmap_text_renderer_smoke ../../Tests/Rendering/bitmap_text_renderer_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/ui_input_router_smoke.cpp")
    add_xpbd_executable(ui_input_router_smoke ../../Tests/Runtime/ui_input_router_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/ui_canvas_renderer_smoke.cpp")
    add_xpbd_executable(ui_canvas_renderer_smoke ../../Tests/Runtime/ui_canvas_renderer_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/ui_layout_resolver_smoke.cpp")
    add_xpbd_executable(ui_layout_resolver_smoke ../../Tests/Runtime/ui_layout_resolver_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/animation_timeline_smoke.cpp")
    add_xpbd_executable(animation_timeline_smoke ../../Tests/Runtime/animation_timeline_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/animation_state_machine_smoke.cpp")
    add_xpbd_executable(animation_state_machine_smoke ../../Tests/Runtime/animation_state_machine_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/animation_locomotion_bridge_smoke.cpp")
    add_xpbd_executable(animation_locomotion_bridge_smoke ../../Tests/Runtime/animation_locomotion_bridge_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/physics_animation_locomotion_bridge_smoke.cpp")
    add_xpbd_executable(physics_animation_locomotion_bridge_smoke ../../Tests/Runtime/physics_animation_locomotion_bridge_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/animation_sprite_tint_binding_smoke.cpp")
    add_xpbd_executable(animation_sprite_tint_binding_smoke ../../Tests/Runtime/animation_sprite_tint_binding_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/flipbook_frame_selector_smoke.cpp")
    add_xpbd_executable(flipbook_frame_selector_smoke ../../Tests/Runtime/flipbook_frame_selector_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/flipbook_playback_smoke.cpp")
    add_xpbd_executable(flipbook_playback_smoke ../../Tests/Runtime/flipbook_playback_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/flipbook_render_bridge_smoke.cpp")
    add_xpbd_executable(flipbook_render_bridge_smoke ../../Tests/Runtime/flipbook_render_bridge_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/animation_playback_control_binding_smoke.cpp")
    add_xpbd_executable(animation_playback_control_binding_smoke ../../Tests/Runtime/animation_playback_control_binding_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/gameplay_physics_query_smoke.cpp")
    add_xpbd_executable(gameplay_physics_query_smoke ../../Tests/Runtime/gameplay_physics_query_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/gameplay_physics_body_smoke.cpp")
    add_xpbd_executable(gameplay_physics_body_smoke ../../Tests/Runtime/gameplay_physics_body_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/kinematic_collision_preflight_smoke.cpp")
    add_xpbd_executable(kinematic_collision_preflight_smoke ../../Tests/Runtime/kinematic_collision_preflight_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/gameplay_trigger_tracker_smoke.cpp")
    add_xpbd_executable(gameplay_trigger_tracker_smoke ../../Tests/Runtime/gameplay_trigger_tracker_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/scene_physics_pose_sync_smoke.cpp")
    add_xpbd_executable(scene_physics_pose_sync_smoke ../../Tests/Runtime/scene_physics_pose_sync_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/texture_import_pipeline_smoke.cpp")
    add_xpbd_executable(texture_import_pipeline_smoke ../../Tests/Runtime/texture_import_pipeline_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/mesh_import_pipeline_smoke.cpp")
    add_xpbd_executable(mesh_import_pipeline_smoke ../../Tests/Runtime/mesh_import_pipeline_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/material_import_pipeline_smoke.cpp")
    add_xpbd_executable(material_import_pipeline_smoke ../../Tests/Runtime/material_import_pipeline_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/transform_animation_binding_smoke.cpp")
    add_xpbd_executable(transform_animation_binding_smoke ../../Tests/Runtime/transform_animation_binding_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Animation/skinning_smoke.cpp")
    add_xpbd_executable(skinning_smoke ../../Tests/Animation/skinning_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Animation/skeleton_smoke.cpp")
    add_xpbd_executable(skeleton_smoke ../../Tests/Animation/skeleton_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Animation/skeletal_pose_clip_smoke.cpp")
    add_xpbd_executable(skeletal_pose_clip_smoke ../../Tests/Animation/skeletal_pose_clip_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Animation/skeletal_pose_player_smoke.cpp")
    add_xpbd_executable(skeletal_pose_player_smoke ../../Tests/Animation/skeletal_pose_player_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Animation/skeletal_animation_controller_smoke.cpp")
    add_xpbd_executable(skeletal_animation_controller_smoke ../../Tests/Animation/skeletal_animation_controller_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Animation/skeletal_locomotion_metadata_smoke.cpp")
    add_xpbd_executable(skeletal_locomotion_metadata_smoke ../../Tests/Animation/skeletal_locomotion_metadata_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/runtime_persistence_smoke.cpp")
    add_xpbd_executable(runtime_persistence_smoke ../../Tests/Runtime/runtime_persistence_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/atomic_save_file_smoke.cpp")
    add_xpbd_executable(atomic_save_file_smoke ../../Tests/Runtime/atomic_save_file_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/android_lifecycle_gate_smoke.cpp")
    add_xpbd_executable(android_lifecycle_gate_smoke ../../Tests/Runtime/android_lifecycle_gate_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/event_signal_bus_smoke.cpp")
    add_xpbd_executable(event_signal_bus_smoke ../../Tests/Runtime/event_signal_bus_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/runtime_authoring_integration_smoke.cpp")
    add_xpbd_executable(runtime_authoring_integration_smoke ../../Tests/Runtime/runtime_authoring_integration_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/runtime_world_vertical_slice_smoke.cpp")
    add_xpbd_executable(runtime_world_vertical_slice_smoke ../../Tests/Runtime/runtime_world_vertical_slice_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/renderer_capability_smoke.cpp")
    add_xpbd_executable(renderer_capability_smoke ../../Tests/Runtime/renderer_capability_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/software_renderer_smoke.cpp")
    add_xpbd_executable(software_renderer_smoke ../../Tests/Runtime/software_renderer_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/software_surface_presenter_smoke.cpp")
    add_xpbd_executable(software_surface_presenter_smoke ../../Tests/Runtime/software_surface_presenter_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/farm_surface_demo_smoke.cpp")
    add_xpbd_executable(farm_surface_demo_smoke ../../Tests/Runtime/farm_surface_demo_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Demos/farm_surface_demo.cpp")
    add_xpbd_executable(farm_surface_demo ../../Demos/farm_surface_demo.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/mesh_surface_demo_smoke.cpp")
    add_xpbd_executable(mesh_surface_demo_smoke ../../Tests/Runtime/mesh_surface_demo_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/asset_import_surface_demo_smoke.cpp")
    add_xpbd_executable(asset_import_surface_demo_smoke ../../Tests/Runtime/asset_import_surface_demo_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/material_import_surface_demo_smoke.cpp")
    add_xpbd_executable(material_import_surface_demo_smoke ../../Tests/Runtime/material_import_surface_demo_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/material_refresh_surface_demo_smoke.cpp")
    add_xpbd_executable(material_refresh_surface_demo_smoke ../../Tests/Runtime/material_refresh_surface_demo_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/motion_animation_surface_demo_smoke.cpp")
    add_xpbd_executable(motion_animation_surface_demo_smoke ../../Tests/Runtime/motion_animation_surface_demo_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/physics_animation_surface_demo_smoke.cpp")
    add_xpbd_executable(physics_animation_surface_demo_smoke ../../Tests/Runtime/physics_animation_surface_demo_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/npc_route_animation_surface_demo_smoke.cpp")
    add_xpbd_executable(npc_route_animation_surface_demo_smoke ../../Tests/Runtime/npc_route_animation_surface_demo_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/flipbook_surface_demo_smoke.cpp")
    add_xpbd_executable(flipbook_surface_demo_smoke ../../Tests/Runtime/flipbook_surface_demo_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/trigger_runtime_surface_demo_smoke.cpp")
    add_xpbd_executable(trigger_runtime_surface_demo_smoke ../../Tests/Runtime/trigger_runtime_surface_demo_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/trigger_animation_surface_demo_smoke.cpp")
    add_xpbd_executable(trigger_animation_surface_demo_smoke ../../Tests/Runtime/trigger_animation_surface_demo_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/kinematic_preflight_surface_demo_smoke.cpp")
    add_xpbd_executable(kinematic_preflight_surface_demo_smoke ../../Tests/Runtime/kinematic_preflight_surface_demo_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Demos/mesh_surface_demo.cpp")
    add_xpbd_executable(mesh_surface_demo ../../Demos/mesh_surface_demo.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/hybrid_surface_demo_smoke.cpp")
    add_xpbd_executable(hybrid_surface_demo_smoke ../../Tests/Runtime/hybrid_surface_demo_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Demos/hybrid_surface_demo.cpp")
    add_xpbd_executable(hybrid_surface_demo ../../Demos/hybrid_surface_demo.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/ppm_texture_smoke.cpp")
    add_xpbd_executable(ppm_texture_smoke ../../Tests/Runtime/ppm_texture_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/bmp_texture_smoke.cpp")
    add_xpbd_executable(bmp_texture_smoke ../../Tests/Runtime/bmp_texture_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/texture_staging_smoke.cpp")
    add_xpbd_executable(texture_staging_smoke ../../Tests/Runtime/texture_staging_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/scene_world_smoke.cpp")
    add_xpbd_executable(scene_world_smoke ../../Tests/Runtime/scene_world_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/editor_scene_document_smoke.cpp")
    add_xpbd_executable(editor_scene_document_smoke ../../Tests/Runtime/editor_scene_document_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/editor_scene_document_codec_smoke.cpp")
    add_xpbd_executable(editor_scene_document_codec_smoke ../../Tests/Runtime/editor_scene_document_codec_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/editor_scene_prefab_smoke.cpp")
    add_xpbd_executable(editor_scene_prefab_smoke ../../Tests/Runtime/editor_scene_prefab_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/editor_scene_prefab_codec_smoke.cpp")
    add_xpbd_executable(editor_scene_prefab_codec_smoke ../../Tests/Runtime/editor_scene_prefab_codec_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/prefab_staging_smoke.cpp")
    add_xpbd_executable(prefab_staging_smoke ../../Tests/Runtime/prefab_staging_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/editor_scene_session_smoke.cpp")
    add_xpbd_executable(editor_scene_session_smoke ../../Tests/Runtime/editor_scene_session_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/editor_scene_session_codec_smoke.cpp")
    add_xpbd_executable(editor_scene_session_codec_smoke ../../Tests/Runtime/editor_scene_session_codec_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/editor_scene_session_dirty_smoke.cpp")
    add_xpbd_executable(editor_scene_session_dirty_smoke ../../Tests/Runtime/editor_scene_session_dirty_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/editor_scene_session_revert_smoke.cpp")
    add_xpbd_executable(editor_scene_session_revert_smoke ../../Tests/Runtime/editor_scene_session_revert_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/editor_scene_session_history_smoke.cpp")
    add_xpbd_executable(editor_scene_session_history_smoke ../../Tests/Runtime/editor_scene_session_history_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/editor_scene_mesh_binder_smoke.cpp")
    add_xpbd_executable(editor_scene_mesh_binder_smoke ../../Tests/Runtime/editor_scene_mesh_binder_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/editor_scene_sprite_binder_smoke.cpp")
    add_xpbd_executable(editor_scene_sprite_binder_smoke ../../Tests/Runtime/editor_scene_sprite_binder_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/authoring_catalog_visual_binder_smoke.cpp")
    add_xpbd_executable(authoring_catalog_visual_binder_smoke ../../Tests/Runtime/authoring_catalog_visual_binder_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/local_authoring_bridge_smoke.cpp")
    add_xpbd_executable(local_authoring_bridge_smoke ../../Tests/Runtime/local_authoring_bridge_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/input_state_smoke.cpp")
    add_xpbd_executable(input_state_smoke ../../Tests/Runtime/input_state_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/sdl_input_bridge_smoke.cpp")
    add_xpbd_executable(sdl_input_bridge_smoke ../../Tests/Runtime/sdl_input_bridge_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/sdl_audio_bridge_smoke.cpp")
    add_xpbd_executable(sdl_audio_bridge_smoke ../../Tests/Runtime/sdl_audio_bridge_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/audio_mixer_smoke.cpp")
    add_xpbd_executable(audio_mixer_smoke ../../Tests/Runtime/audio_mixer_smoke.cpp)
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
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_texture_upload_smoke.cpp")
    add_xpbd_executable(vulkan_texture_upload_smoke ../../Tests/Runtime/vulkan_texture_upload_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_textured_offscreen_smoke.cpp")
    add_xpbd_executable(vulkan_textured_offscreen_smoke ../../Tests/Runtime/vulkan_textured_offscreen_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/vulkan_present_smoke.cpp")
    add_xpbd_executable(vulkan_present_smoke ../../Tests/Runtime/vulkan_present_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/farm_render_smoke.cpp")
    add_xpbd_executable(farm_render_smoke ../../Tests/Runtime/farm_render_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/farm_world_render_smoke.cpp")
    add_xpbd_executable(farm_world_render_smoke ../../Tests/Runtime/farm_world_render_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/farm_sprite_render_smoke.cpp")
    add_xpbd_executable(farm_sprite_render_smoke ../../Tests/Runtime/farm_sprite_render_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/farm_player_input_bridge_smoke.cpp")
    add_xpbd_executable(farm_player_input_bridge_smoke ../../Tests/Runtime/farm_player_input_bridge_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/farm_action_panel_controller_smoke.cpp")
    add_xpbd_executable(farm_action_panel_controller_smoke ../../Tests/Runtime/farm_action_panel_controller_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/farm_runtime_session_smoke.cpp")
    add_xpbd_executable(farm_runtime_session_smoke ../../Tests/Runtime/farm_runtime_session_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/farm_runtime_hud_smoke.cpp")
    add_xpbd_executable(farm_runtime_hud_smoke ../../Tests/Runtime/farm_runtime_hud_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/farm_interactive_hud_smoke.cpp")
    add_xpbd_executable(farm_interactive_hud_smoke ../../Tests/Runtime/farm_interactive_hud_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/farm_interactive_surface_demo_smoke.cpp")
    add_xpbd_executable(farm_interactive_surface_demo_smoke ../../Tests/Runtime/farm_interactive_surface_demo_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/runtime_farm_hud_smoke.cpp")
    add_xpbd_executable(runtime_farm_hud_smoke ../../Tests/Runtime/runtime_farm_hud_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/farm_runtime_save_codec_smoke.cpp")
    add_xpbd_executable(farm_runtime_save_codec_smoke ../../Tests/Runtime/farm_runtime_save_codec_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/farm_runtime_session_checkpoint_smoke.cpp")
    add_xpbd_executable(farm_runtime_session_checkpoint_smoke ../../Tests/Runtime/farm_runtime_session_checkpoint_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/farm_runtime_session_world_checkpoint_smoke.cpp")
    add_xpbd_executable(farm_runtime_session_world_checkpoint_smoke ../../Tests/Runtime/farm_runtime_session_world_checkpoint_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/farm_runtime_session_hud_smoke.cpp")
    add_xpbd_executable(farm_runtime_session_hud_smoke ../../Tests/Runtime/farm_runtime_session_hud_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/asset_registry_smoke.cpp")
    add_xpbd_executable(asset_registry_smoke ../../Tests/Runtime/asset_registry_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/asset_manifest_snapshot_smoke.cpp")
    add_xpbd_executable(asset_manifest_snapshot_smoke ../../Tests/Runtime/asset_manifest_snapshot_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/asset_reload_diagnostics_smoke.cpp")
    add_xpbd_executable(asset_reload_diagnostics_smoke ../../Tests/Runtime/asset_reload_diagnostics_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/asset_refresh_diagnostics_smoke.cpp")
    add_xpbd_executable(asset_refresh_diagnostics_smoke ../../Tests/Runtime/asset_refresh_diagnostics_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/asset_refresh_executor_smoke.cpp")
    add_xpbd_executable(asset_refresh_executor_smoke ../../Tests/Runtime/asset_refresh_executor_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/legacy_engine_loop_smoke.cpp")
    add_xpbd_executable(legacy_engine_loop_smoke ../../Tests/Runtime/legacy_engine_loop_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Systems/trust_safety_smoke.cpp")
    add_xpbd_executable(trust_safety_smoke ../../Tests/Systems/trust_safety_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Systems/telemetry_outbox_smoke.cpp")
    add_xpbd_executable(telemetry_outbox_smoke ../../Tests/Systems/telemetry_outbox_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Systems/authoritative_command_gate_smoke.cpp")
    add_xpbd_executable(authoritative_command_gate_smoke ../../Tests/Systems/authoritative_command_gate_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Systems/authority_wire_protocol_smoke.cpp")
    add_xpbd_executable(authority_wire_protocol_smoke ../../Tests/Systems/authority_wire_protocol_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Systems/authority_loopback_transport_smoke.cpp")
    add_xpbd_executable(authority_loopback_transport_smoke ../../Tests/Systems/authority_loopback_transport_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Systems/authority_scale_smoke.cpp")
    add_xpbd_executable(authority_scale_smoke ../../Tests/Systems/authority_scale_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Systems/authoring_catalog_smoke.cpp")
    add_xpbd_executable(authoring_catalog_smoke ../../Tests/Systems/authoring_catalog_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Systems/commodity_catalog_smoke.cpp")
    add_xpbd_executable(commodity_catalog_smoke ../../Tests/Systems/commodity_catalog_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Systems/commodity_economy_ledger_smoke.cpp")
    add_xpbd_executable(commodity_economy_ledger_smoke ../../Tests/Systems/commodity_economy_ledger_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Systems/game_loan_book_smoke.cpp")
    add_xpbd_executable(game_loan_book_smoke ../../Tests/Systems/game_loan_book_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Systems/grid_navigation_smoke.cpp")
    add_xpbd_executable(grid_navigation_smoke ../../Tests/Systems/grid_navigation_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Systems/world_authoring_smoke.cpp")
    add_xpbd_executable(world_authoring_smoke ../../Tests/Systems/world_authoring_smoke.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Physics/xpbd_regression.cpp")
    add_xpbd_executable(xpbd_regression ../../Tests/Physics/xpbd_regression.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Physics/xpbd_determinism.cpp")
    add_xpbd_executable(xpbd_determinism ../../Tests/Physics/xpbd_determinism.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/kinematic_motion_controller_smoke.cpp")
    add_xpbd_executable(kinematic_motion_controller_smoke ../../Tests/Runtime/kinematic_motion_controller_smoke.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/input_motion_bridge_smoke.cpp")
    add_xpbd_executable(input_motion_bridge_smoke ../../Tests/Runtime/input_motion_bridge_smoke.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/runtime_input_motion_smoke.cpp")
    add_xpbd_executable(runtime_input_motion_smoke ../../Tests/Runtime/runtime_input_motion_smoke.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/grid_route_follower_smoke.cpp")
    add_xpbd_executable(grid_route_follower_smoke ../../Tests/Runtime/grid_route_follower_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/grid_route_intent_smoke.cpp")
    add_xpbd_executable(grid_route_intent_smoke ../../Tests/Runtime/grid_route_intent_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/route_root_motion_adapter_smoke.cpp")
    add_xpbd_executable(route_root_motion_adapter_smoke ../../Tests/Runtime/route_root_motion_adapter_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/movement_authority_smoke.cpp")
    add_xpbd_executable(movement_authority_smoke ../../Tests/Runtime/movement_authority_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/runtime_route_motion_smoke.cpp")
    add_xpbd_executable(runtime_route_motion_smoke ../../Tests/Runtime/runtime_route_motion_smoke.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Tests/Runtime/runtime_route_root_motion_smoke.cpp")
    add_xpbd_executable(runtime_route_root_motion_smoke ../../Tests/Runtime/runtime_route_root_motion_smoke.cpp)
endif()
