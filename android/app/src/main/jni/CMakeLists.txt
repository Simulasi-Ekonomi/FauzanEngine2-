cmake_minimum_required(VERSION 3.22.1)
project(NeoEngineAndroidCanonical LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Deliberately compile only the audited canonical core subset. This is not a
# complete Android game client and does not package an APK/AAB.
set(NEO_ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Source/NeoEngine")
set(NEO_CANONICAL_ANDROID_SOURCES
    "${NEO_ENGINE_ROOT}/Runtime/RuntimeClock.cpp"
    "${NEO_ENGINE_ROOT}/Runtime/RuntimePersistence.cpp"
    "${NEO_ENGINE_ROOT}/Runtime/AtomicSaveFile.cpp"
    "${NEO_ENGINE_ROOT}/Runtime/AndroidLifecycleGate.cpp"
    "${NEO_ENGINE_ROOT}/Systems/CommodityCatalog.cpp"
    "${NEO_ENGINE_ROOT}/Systems/CommodityEconomyLedger.cpp"
    "${NEO_ENGINE_ROOT}/Systems/GameLoanBook.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/NeoEngineCanonicalBridge.cpp"
)

foreach(source_file IN LISTS NEO_CANONICAL_ANDROID_SOURCES)
    if(NOT EXISTS "${source_file}")
        message(FATAL_ERROR "Canonical Android source missing: ${source_file}")
    endif()
endforeach()

add_library(neo_core SHARED ${NEO_CANONICAL_ANDROID_SOURCES})
target_include_directories(neo_core PRIVATE "${NEO_ENGINE_ROOT}")
target_compile_definitions(neo_core PRIVATE ANDROID NEO_PLATFORM_ANDROID=1 NEO_CANONICAL_ANDROID_SUBSET=1)

find_library(log-lib log)
find_library(android-lib android)
target_link_libraries(neo_core PRIVATE ${log-lib} ${android-lib})

message(STATUS "NeoEngine canonical Android subset sources: ${NEO_CANONICAL_ANDROID_SOURCES}")
