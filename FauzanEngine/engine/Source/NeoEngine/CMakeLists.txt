# =========================================================
# NeoEngine Core - CMakeLists.txt (FIXED)
# Fauzan Engine v1.0
# =========================================================

file(GLOB_RECURSE NEO_CORE_SRC
    "*.cpp"
    "Core/*.cpp"
    "Core/Memory/*.cpp"
    "Core/Debug/*.cpp"
    "Core/Log/*.cpp"
    "Core/Math/*.cpp"
    "Core/Object/*.cpp"
    "ECS/*.cpp"
    "Platform/*.cpp"
    "Platform/Common/*.cpp"
    "Platform/Desktop/*.cpp"
    "Platform/Android/*.cpp"
    "Rendering/*.cpp"
    "Rendering/RHI/*.cpp"
    "Rendering/RHI/Vulkan/*.cpp"
    "Rendering/Renderer/*.cpp"
    "Rendering/Shader/*.cpp"
    "World/*.cpp"
    "Editor/*.cpp"
    "Memory/*.cpp"
    "Actor/*.cpp"
    "Asset/*.cpp"
    "Component/*.cpp"
    "Cooker/*.cpp"
    "Gizmo/*.cpp"
    "Importer/*.cpp"
    "Level/*.cpp"
    "Materials/*.cpp"
    "Object/*.cpp"
    "RHI/*.cpp"
    "Reflection/*.cpp"
    "Serializer/*.cpp"
    "Threading/*.cpp"
    "Viewport/*.cpp"
    "Autonomous/*.cpp"
)

add_library(neo_core SHARED ${NEO_CORE_SRC})

# Include directories - all source subdirectories
target_include_directories(neo_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Memory
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Debug
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Log
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Math
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Object
    ${CMAKE_CURRENT_SOURCE_DIR}/ECS
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Common
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Desktop
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Android
    ${CMAKE_CURRENT_SOURCE_DIR}/Rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/Rendering/RHI
    ${CMAKE_CURRENT_SOURCE_DIR}/Rendering/RHI/Vulkan
    ${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Shader
    ${CMAKE_CURRENT_SOURCE_DIR}/World
    ${CMAKE_CURRENT_SOURCE_DIR}/Editor
    ${CMAKE_CURRENT_SOURCE_DIR}/Memory
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor
    ${CMAKE_CURRENT_SOURCE_DIR}/Asset
    ${CMAKE_CURRENT_SOURCE_DIR}/Component
    ${CMAKE_CURRENT_SOURCE_DIR}/Cooker
    ${CMAKE_CURRENT_SOURCE_DIR}/Gizmo
    ${CMAKE_CURRENT_SOURCE_DIR}/Importer
    ${CMAKE_CURRENT_SOURCE_DIR}/Level
    ${CMAKE_CURRENT_SOURCE_DIR}/Materials
    ${CMAKE_CURRENT_SOURCE_DIR}/Object
    ${CMAKE_CURRENT_SOURCE_DIR}/RHI
    ${CMAKE_CURRENT_SOURCE_DIR}/Reflection
    ${CMAKE_CURRENT_SOURCE_DIR}/Serializer
    ${CMAKE_CURRENT_SOURCE_DIR}/Threading
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewport
    ${CMAKE_CURRENT_SOURCE_DIR}/Autonomous
)

# Platform-specific linking
if(ANDROID)
    find_library(log-lib log)
    if(log-lib)
        target_link_libraries(neo_core ${log-lib})
    endif()
    target_link_libraries(neo_core android log)
    target_include_directories(neo_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Android
    )
elseif(WIN32)
    target_include_directories(neo_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Windows
    )
elseif(UNIX)
    target_include_directories(neo_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Desktop
    )
endif()

set_target_properties(neo_core PROPERTIES OUTPUT_NAME "neo_core")

# Vulkan (optional)
find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    target_link_libraries(neo_core Vulkan::Vulkan)
    target_compile_definitions(neo_core PRIVATE NEO_VULKAN_ENABLED=1)
    message(STATUS "Vulkan found - RHI enabled")
else()
    message(STATUS "Vulkan not found - RHI disabled (software rendering)")
endif()

message(STATUS "NeoEngine Core configured with ${CMAKE_CXX_STANDARD} standard")
message(STATUS "Source files: ${NEO_CORE_SRC}")
