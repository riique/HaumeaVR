add_library(
    uxrb_unity_bridge
    SHARED
    src/unity_bridge.cpp
)

target_include_directories(
    uxrb_unity_bridge
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${UXRB_UNITY_NATIVE_INTERFACE_DIR}
)

target_compile_definitions(
    uxrb_unity_bridge
    PRIVATE
        UXRB_UNITY_BRIDGE_EXPORTS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
)

target_link_libraries(uxrb_unity_bridge PRIVATE uxrb_unity_transport d3d11 dxgi)

if(MSVC)
    target_compile_options(
        uxrb_unity_bridge
        PRIVATE
            /W4
            /permissive-
            /Zc:preprocessor
            /utf-8
    )
endif()

set_target_properties(
    uxrb_unity_bridge
    PROPERTIES
        OUTPUT_NAME "uxrb_unity_bridge"
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:uxrb_unity_bridge>"
)
