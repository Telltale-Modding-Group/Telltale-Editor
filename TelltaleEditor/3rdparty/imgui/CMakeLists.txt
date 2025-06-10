cmake_minimum_required(VERSION 3.16)

set(imgui_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)


set(CMAKE_CXX_STANDARD 17)

# Add ImGui static library
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/imgui/backends/imgui_impl_sdlgpu3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}/imgui
    ${imgui_SOURCE_DIR}/imgui/backends
)

target_link_libraries(imgui PRIVATE
    SDL3::SDL3-static
)

ignore_warnings(imgui 0)