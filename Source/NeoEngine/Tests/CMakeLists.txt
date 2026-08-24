cmake_minimum_required(VERSION 3.22)
project(NeoEngineBench LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NEO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(BENCH_SRC
    ${NEO_ROOT}/Tests/BenchmarkMain.cpp
    ${NEO_ROOT}/Core/Memory/MemoryManager.cpp
    ${NEO_ROOT}/World/NeoWorld.cpp
    ${NEO_ROOT}/ECS/EntityManager.cpp
    ${NEO_ROOT}/Systems/RPGSystem.cpp
    ${NEO_ROOT}/Systems/CraftingSystem.cpp
    ${NEO_ROOT}/Systems/InventorySystem.cpp
)

set(BENCH_HEADERS
    ${NEO_ROOT}/Systems/CombatSystem.h
    ${NEO_ROOT}/Systems/FarmingSystem.h
)

add_executable(neo_benchmark ${BENCH_SRC} ${BENCH_HEADERS})

target_include_directories(neo_benchmark PRIVATE
    ${NEO_ROOT}
    ${NEO_ROOT}/Core
    ${NEO_ROOT}/Core/Memory
    ${NEO_ROOT}/ECS
    ${NEO_ROOT}/World
    ${NEO_ROOT}/Systems
)

# Arahkan linker ke folder library Termux
link_directories(/data/data/com.termux/files/usr/lib)

# Link dengan nama library saja (tanpa path)
target_link_libraries(neo_benchmark log android)
