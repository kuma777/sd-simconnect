cmake_minimum_required(VERSION 3.27)

project(sd-simconnect CXX)

add_subdirectory(
    lib/glfw
)

include_directories(
    "C:/MSFS SDK/SimConnect SDK/include"
    lib
    lib/glfw/include
    lib/imgui
)

link_directories(
    "C:/MSFS SDK/SimConnect SDK/lib"
)

add_library(imgui
    lib/imgui/backends/imgui_impl_glfw.cpp
    lib/imgui/backends/imgui_impl_opengl3.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/imgui.cpp
)

add_executable(sd-simconnect WIN32 main.cpp)

target_link_libraries(sd-simconnect opengl32 glfw imgui Websocket SimConnect)