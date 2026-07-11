add_library(
    uxrb_d3d11_intercept
    SHARED
    src/d3d11_intercept.cpp
)

target_include_directories(
    uxrb_d3d11_intercept
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(
    uxrb_d3d11_intercept
    PRIVATE
        UXRB_D3D11_INTERCEPT_EXPORTS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
)

target_link_libraries(
    uxrb_d3d11_intercept
    PRIVATE
        uxrb_detours
        d3d11
        dxgi
        user32
)

if(MSVC)
    target_compile_options(
        uxrb_d3d11_intercept
        PRIVATE
            /W4
            /permissive-
            /Zc:preprocessor
            /utf-8
    )
endif()

set_target_properties(
    uxrb_d3d11_intercept
    PROPERTIES
        OUTPUT_NAME "uxrb_d3d11_intercept"
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:uxrb_d3d11_intercept>"
)
