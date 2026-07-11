add_library(
    uxrb_ipc
    STATIC
    src/ipc_protocol.cpp
    src/named_pipe.cpp
)

target_include_directories(
    uxrb_ipc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(
    uxrb_ipc
    PUBLIC
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
)

target_link_libraries(
    uxrb_ipc
    PUBLIC
        nlohmann_json::nlohmann_json
    PRIVATE
        bcrypt
)

if(MSVC)
    target_compile_options(uxrb_ipc PRIVATE /W4 /permissive- /Zc:preprocessor /utf-8)
endif()

