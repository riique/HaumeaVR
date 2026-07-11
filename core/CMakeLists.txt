add_library(
    uxrb_runtime
    STATIC
    src/desktop_capture.cpp
    src/d3d11_renderer.cpp
    src/depth_reconstruction.cpp
    src/logger.cpp
    src/mirror_window.cpp
    src/openxr_app.cpp
    src/projection.cpp
    src/unity_shared_texture_source.cpp
)

target_include_directories(
    uxrb_runtime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(
    uxrb_runtime
    PUBLIC
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
        XR_USE_PLATFORM_WIN32
        XR_USE_GRAPHICS_API_D3D11
)

target_link_libraries(
    uxrb_runtime
    PUBLIC
        openxr_loader
        uxrb_unity_transport
    PRIVATE
        advapi32
        d3d11
        d3dcompiler
        dxgi
        user32
)

if(MSVC)
    target_compile_options(
        uxrb_runtime
        PRIVATE
            /W4
            /permissive-
            /Zc:preprocessor
            /utf-8
    )
endif()

add_executable(uxrb_core src/ipc_server.cpp src/main.cpp)
target_link_libraries(uxrb_core PRIVATE uxrb_runtime uxrb_ipc uxrb_unity_transport)
target_compile_definitions(uxrb_core PRIVATE UXRB_VERSION="${PROJECT_VERSION}")

if(MSVC)
    target_compile_options(uxrb_core PRIVATE /W4 /permissive- /utf-8)
endif()

set_target_properties(
    uxrb_core
    PROPERTIES
        OUTPUT_NAME "uxrb_core"
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:uxrb_core>"
)
