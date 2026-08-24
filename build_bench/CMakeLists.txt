cmake_minimum_required(VERSION 3.22)
project(AITactical LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=armv8-a+simd")
add_executable(ai_test
    ../Source/NeoEngine/Core/ECS/EntityManager.cpp
    ../Source/NeoEngine/Physics/RigidBody.cpp
    ../Source/NeoEngine/Physics/CollisionSAT.cpp
    ../Tests/AI/test_ai_tactical.cpp
)
target_include_directories(ai_test PRIVATE
    ../Source/NeoEngine ../Source/NeoEngine/Core ../Source/NeoEngine/ECS
    ../Source/NeoEngine/AI ../Source/NeoEngine/Physics ../Source/NeoEngine/Systems
    ../Source/NeoEngine/World
)
