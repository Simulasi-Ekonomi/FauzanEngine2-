cmake_minimum_required(VERSION 3.22)
project(NeoEngineCore LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Tambahkan direktori source
add_subdirectory(Source/NeoEngine)
