include(FetchContent)

# The SDK repository contains Khronos' generated headers and official loader.
# Keep optional SDK products disabled so the bridge only builds what it uses.
set(BUILD_API_LAYERS OFF CACHE BOOL "" FORCE)
set(BUILD_CONFORMANCE_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SDK_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_LOADER ON CACHE BOOL "" FORCE)
set(BUILD_ALL_EXTENSIONS OFF CACHE BOOL "" FORCE)
set(DYNAMIC_LOADER ON CACHE BOOL "" FORCE)

# The Milestone 0 bridge is D3D11-only. Avoid probing unrelated graphics SDKs
# while configuring the Khronos loader dependency.
set(CMAKE_DISABLE_FIND_PACKAGE_OpenGL TRUE CACHE BOOL "" FORCE)
set(CMAKE_DISABLE_FIND_PACKAGE_Vulkan TRUE CACHE BOOL "" FORCE)

FetchContent_Declare(
    OpenXRSDK
    GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK.git
    GIT_TAG ${UXRB_OPENXR_SDK_TAG}
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(OpenXRSDK)

if(NOT TARGET openxr_loader)
    message(FATAL_ERROR "The pinned OpenXR SDK did not provide the openxr_loader target.")
endif()

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

if(UXRB_BUILD_UNITY_BRIDGE)
    # Unity publishes its native plug-in interface headers in this MIT-licensed
    # reference repository. Pinning the exact commit keeps the bridge ABI inputs
    # reproducible instead of relying on headers copied from a local Editor.
    FetchContent_Declare(
        UnityNativeRenderingPlugin
        GIT_REPOSITORY https://github.com/Unity-Technologies/NativeRenderingPlugin.git
        GIT_TAG ${UXRB_UNITY_NATIVE_PLUGIN_COMMIT}
        GIT_SHALLOW FALSE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(UnityNativeRenderingPlugin)

    set(
        UXRB_UNITY_NATIVE_INTERFACE_DIR
        "${unitynativerenderingplugin_SOURCE_DIR}/PluginSource/source"
        CACHE INTERNAL
        "Unity native plug-in interface include directory"
    )
endif()

if(UXRB_BUILD_D3D11_INTERCEPT)
    FetchContent_Declare(
        MicrosoftDetours
        GIT_REPOSITORY https://github.com/microsoft/Detours.git
        GIT_TAG ${UXRB_DETOURS_TAG}
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(MicrosoftDetours)

    add_library(
        uxrb_detours
        STATIC
        ${microsoftdetours_SOURCE_DIR}/src/detours.cpp
        ${microsoftdetours_SOURCE_DIR}/src/modules.cpp
        ${microsoftdetours_SOURCE_DIR}/src/disasm.cpp
        ${microsoftdetours_SOURCE_DIR}/src/image.cpp
        ${microsoftdetours_SOURCE_DIR}/src/disolx86.cpp
        ${microsoftdetours_SOURCE_DIR}/src/disolx64.cpp
        ${microsoftdetours_SOURCE_DIR}/src/disolia64.cpp
        ${microsoftdetours_SOURCE_DIR}/src/disolarm.cpp
        ${microsoftdetours_SOURCE_DIR}/src/disolarm64.cpp
    )
    # creatwth.cpp intentionally stays out of this narrow library: UXRB does
    # not ship Detours' process-creation/injection helpers. The module can only
    # be installed explicitly inside its current process.
    target_include_directories(uxrb_detours PUBLIC ${microsoftdetours_SOURCE_DIR}/src)
    target_compile_definitions(uxrb_detours PRIVATE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601)
    if(MSVC)
        target_compile_options(uxrb_detours PRIVATE /W3 /utf-8 /wd4100 /wd4127 /wd4242 /wd4244 /wd4456 /wd4702)
    endif()
endif()
