cmake_minimum_required(VERSION 3.22.1)
project(NeoEngineAndroid LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NEO_ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../engine/Source/NeoEngine")
message(STATUS "Engine root: ${NEO_ENGINE_ROOT}")

file(GLOB_RECURSE NEO_CORE_SRC
    "${NEO_ENGINE_ROOT}/Core/*.cpp"
    "${NEO_ENGINE_ROOT}/ECS/*.cpp"
    "${NEO_ENGINE_ROOT}/Platform/Android/*.cpp"
    "${NEO_ENGINE_ROOT}/Platform/Common/*.cpp"
    "${NEO_ENGINE_ROOT}/Rendering/**/*.cpp"
    "${NEO_ENGINE_ROOT}/Renderer/*.cpp"
    "${NEO_ENGINE_ROOT}/World/*.cpp"
    "${NEO_ENGINE_ROOT}/Editor/*.cpp"
    "${NEO_ENGINE_ROOT}/Memory/*.cpp"
    "${NEO_ENGINE_ROOT}/AI/*.cpp"
    "${NEO_ENGINE_ROOT}/Animation/*.cpp"
    "${NEO_ENGINE_ROOT}/Physics/*.cpp"
    "${NEO_ENGINE_ROOT}/Asset/*.cpp"
    "${NEO_ENGINE_ROOT}/Threading/*.cpp"
    "${NEO_ENGINE_ROOT}/Scheduler/*.cpp"
    "${NEO_ENGINE_ROOT}/Component/*.cpp"
    "${NEO_ENGINE_ROOT}/Actor/*.cpp"
    "${NEO_ENGINE_ROOT}/Level/*.cpp"
    "${NEO_ENGINE_ROOT}/Scene/*.cpp"
    "${NEO_ENGINE_ROOT}/Viewport/*.cpp"
    "${NEO_ENGINE_ROOT}/Serializer/*.cpp"
    "${NEO_ENGINE_ROOT}/Reflection/*.cpp"
    "${NEO_ENGINE_ROOT}/Networking/*.cpp"
    "${NEO_ENGINE_ROOT}/Autonomous/*.cpp"
    "${NEO_ENGINE_ROOT}/*.cpp"
)

# Exclude desktop platform
list(FILTER NEO_CORE_SRC EXCLUDE REGEX ".*/Platform/Desktop/.*")
list(FILTER NEO_CORE_SRC EXCLUDE REGEX ".*/build/.*")
list(FILTER NEO_CORE_SRC EXCLUDE REGEX ".*cube_with_normals.*")
list(FILTER NEO_CORE_SRC EXCLUDE REGEX ".*/main\\.cpp")

file(GLOB JNI_BRIDGE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(neo_core SHARED
    ${NEO_CORE_SRC}
    ${JNI_BRIDGE_SRC}
)

target_include_directories(neo_core PUBLIC
    ${NEO_ENGINE_ROOT}
    ${NEO_ENGINE_ROOT}/Core
    ${NEO_ENGINE_ROOT}/Core/Math
    ${NEO_ENGINE_ROOT}/Core/Memory
    ${NEO_ENGINE_ROOT}/Core/Debug
    ${NEO_ENGINE_ROOT}/Core/Object
    ${NEO_ENGINE_ROOT}/ECS
    ${NEO_ENGINE_ROOT}/Platform
    ${NEO_ENGINE_ROOT}/Platform/Android
    ${NEO_ENGINE_ROOT}/Platform/Common
    ${NEO_ENGINE_ROOT}/Rendering
    ${NEO_ENGINE_ROOT}/Rendering/RHI
    ${NEO_ENGINE_ROOT}/Rendering/Renderer
    ${NEO_ENGINE_ROOT}/World
    ${NEO_ENGINE_ROOT}/Editor
    ${NEO_ENGINE_ROOT}/Memory
    ${NEO_ENGINE_ROOT}/AI
    ${NEO_ENGINE_ROOT}/Animation
    ${NEO_ENGINE_ROOT}/Physics
    ${NEO_ENGINE_ROOT}/Asset
    ${NEO_ENGINE_ROOT}/Threading
    ${NEO_ENGINE_ROOT}/Scheduler
    ${NEO_ENGINE_ROOT}/Component
    ${NEO_ENGINE_ROOT}/Actor
    ${NEO_ENGINE_ROOT}/Level
    ${NEO_ENGINE_ROOT}/Scene
    ${NEO_ENGINE_ROOT}/Viewport
    ${NEO_ENGINE_ROOT}/Serializer
    ${NEO_ENGINE_ROOT}/Reflection
    ${NEO_ENGINE_ROOT}/Networking
    ${NEO_ENGINE_ROOT}/Autonomous
)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(neo_core ${log-lib} ${android-lib})

target_compile_definitions(neo_core PRIVATE
    ANDROID
    NEO_PLATFORM_ANDROID=1
    __ARM_NEON=1
)

find_library(vulkan-lib vulkan)
if(vulkan-lib)
    target_link_libraries(neo_core ${vulkan-lib})
    target_compile_definitions(neo_core PRIVATE NEO_VULKAN_ENABLED=1)
endif()

list(LENGTH NEO_CORE_SRC SRC_COUNT)
message(STATUS "NeoEngine: ${SRC_COUNT} source files for ${ANDROID_ABI}")
