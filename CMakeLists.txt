cmake_minimum_required(VERSION 3.28)

project(
    HaumeaVR
    VERSION 0.1.0
    DESCRIPTION "HaumeaVR OpenXR platform for bringing flat-screen games into VR"
    LANGUAGES C CXX
)

if(NOT WIN32)
    message(FATAL_ERROR "HaumeaVR currently supports Windows only.")
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are disabled. Use a preset from CMakePresets.json.")
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Native binaries are shipped beside a self-contained .NET launcher. Link the
# MSVC runtime statically so the portable bundle does not silently depend on a
# machine-wide Visual C++ Redistributable installation.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

option(UXRB_BUILD_TESTS "Build the runtime-independent test suite" ON)
option(UXRB_BUILD_UNITY_BRIDGE "Build the opt-in Unity D3D11 native bridge" ON)
option(UXRB_BUILD_D3D11_INTERCEPT "Build the opt-in current-process D3D11 observation module" ON)
set(UXRB_OPENXR_SDK_TAG "release-1.1.58" CACHE STRING "Pinned Khronos OpenXR-SDK tag")
set(
    UXRB_UNITY_NATIVE_PLUGIN_COMMIT
    "f703c47a140d343c2c863a36d1aa5832586f3aaa"
    CACHE STRING
    "Pinned Unity NativeRenderingPlugin commit used for official native interfaces"
)
set(UXRB_DETOURS_TAG "v4.0.1" CACHE STRING "Pinned Microsoft Detours stable tag")

include(cmake/Dependencies.cmake)

add_subdirectory(shared/ipc)
add_subdirectory(shared/unity_transport)
add_subdirectory(core)

if(UXRB_BUILD_UNITY_BRIDGE)
    add_subdirectory(engines/unity/native)
endif()

if(UXRB_BUILD_D3D11_INTERCEPT)
    add_subdirectory(graphics/intercept/d3d11)
endif()

include(CTest)
if(BUILD_TESTING AND UXRB_BUILD_TESTS)
    add_subdirectory(tests)
endif()
