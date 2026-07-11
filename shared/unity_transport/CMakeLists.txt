add_library(
    uxrb_unity_transport
    STATIC
    src/unity_transport.cpp
)

target_include_directories(
    uxrb_unity_transport
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(
    uxrb_unity_transport
    PUBLIC
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
)

if(MSVC)
    target_compile_options(uxrb_unity_transport PRIVATE /W4 /permissive- /utf-8)
endif()
