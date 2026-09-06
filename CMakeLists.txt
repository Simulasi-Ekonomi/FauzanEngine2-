cmake_minimum_required(VERSION 3.18)
project(NeoEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Collect all engine headers
file(GLOB_RECURSE NEO_ENGINE_HEADERS "include/NeoEngine/*.hpp")

# Define Header-Only Interface Library
add_library(NeoEngineCore INTERFACE)

target_include_directories(NeoEngineCore INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_sources(NeoEngineCore INTERFACE ${NEO_ENGINE_HEADERS})

# Smoke Test Executable
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp")
    add_executable(NeoEngineSmokeTest tests/main.cpp)
    target_link_libraries(NeoEngineSmokeTest PRIVATE NeoEngineCore)
    
    if(MSVC)
        target_compile_options(NeoEngineSmokeTest PRIVATE /W4 /WX)
    else()
        target_compile_options(NeoEngineSmokeTest PRIVATE -Wall -Wextra -Werror -pedantic)
    endif()
endif()

message(STATUS "NeoEngineCore Interface Target Configured Successfully.")
