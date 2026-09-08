find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

link_libraries(
    ${CURL_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    log
)

cmake_minimum_required(VERSION 3.28)

project(NeoEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(XPBD_RUNTIME_SOURCES
    Runtime/AtomicSaveFile.cpp
    Systems/AuthorityLoopbackServer.cpp
    Systems/ItemSerialTracker.cpp
    Systems/TelemetryOutbox.cpp
    Systems/TrustSafetySystem.cpp
    Templates/GameTemplateRegistry.cpp
    Templates/MatchThreeGame.cpp
    Templates/RpgSandboxGame.cpp
    Templates/SudokuGame.cpp
    Templates/TowerDefenseGame.cpp
    Threading/JobSystem.cpp
    Rendering/Rendering/Renderer/GPUDrivenRenderer.cpp
    Rendering/Rendering/Renderer/GPUFrustumCulling.cpp
    Rendering/Rendering/Renderer/HiZBuffer.cpp
    Rendering/Rendering/Renderer/OcclusionCulling.cpp
)

add_library(NeoEngineRuntime STATIC ${XPBD_RUNTIME_SOURCES})
target_include_directories(NeoEngineRuntime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

function(add_xpbd_executable target_name source_file)
    add_executable(${target_name} ${source_file})
    target_link_libraries(${target_name} PRIVATE NeoEngineRuntime)
endfunction()

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

# R3 GPU Indirect Executables
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Tests/r3_gpu_indirect_10k_execution_smoke.cpp")
    add_xpbd_executable(r3_gpu_indirect_10k_execution_smoke Tests/r3_gpu_indirect_10k_execution_smoke.cpp)
    add_xpbd_executable(r3_gpu_indirect_capacity_smoke Tests/r3_gpu_indirect_capacity_smoke.cpp)
    add_xpbd_executable(gpu_driven_renderer_smoke Tests/gpu_driven_renderer_smoke.cpp)
    add_xpbd_executable(vulkan_mesh_indirect_batch_smoke Tests/vulkan_mesh_indirect_batch_smoke.cpp)
    add_xpbd_executable(vulkan_mesh_indirect_batch_gpu_smoke Tests/vulkan_mesh_indirect_batch_gpu_smoke.cpp)
endif()

# R4 Frustum Culling Executables
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Tests/r4_frustum_culling_smoke.cpp")
    add_xpbd_executable(r4_frustum_culling_smoke Tests/r4_frustum_culling_smoke.cpp)
    add_xpbd_executable(r4_occlusion_hiz_smoke Tests/r4_occlusion_hiz_smoke.cpp)
endif()

message(STATUS "✅ NeoEngine CMake configured with native nested source paths")

# Jules Release Gates R6-R10
add_xpbd_executable(r6_anti_cheat_100_smoke ../../Tests/r6_anti_cheat_100_smoke.cpp)
add_xpbd_executable(r7_economy_commerce_100_smoke ../../Tests/r7_economy_commerce_100_smoke.cpp)
add_xpbd_executable(r8_persistence_recovery_100_smoke ../../Tests/r8_persistence_recovery_100_smoke.cpp)
add_xpbd_executable(r9_live_ops_telemetry_100_smoke ../../Tests/r9_live_ops_telemetry_100_smoke.cpp)
add_xpbd_executable(r10_security_privacy_100_smoke ../../Tests/r10_security_privacy_100_smoke.cpp)
add_xpbd_executable(comprehensive_engine_verification_smoke ../../Tests/comprehensive_engine_verification_smoke.cpp)
