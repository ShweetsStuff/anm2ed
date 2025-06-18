cmake_minimum_required(VERSION 3.10)
project(anm2ed CXX)

# Gather project sources
file(GLOB SOURCES
    "include/imgui/imgui.cpp"
    "include/imgui/imgui_draw.cpp"
    "include/imgui/imgui_tables.cpp"
    "include/imgui/imgui_widgets.cpp"
    "include/imgui/backends/imgui_impl_sdl3.cpp"
    "include/imgui/backends/imgui_impl_opengl3.cpp"
    "include/tinyxml2/tinyxml2.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE include include/imgui include/tinyxml2 src)

set(CMAKE_CXX_FLAGS "-g -O2 -std=c++23 -Wall -Wextra -pedantic -Wno-unused-variable -Wno-unused-parameter -Wno-ignored-qualifiers -Wno-parentheses -Wno-unused-function -Wno-class-memaccess -Wno-delete-incomplete")

target_link_libraries(${PROJECT_NAME} PRIVATE m GL GLEW SDL3)

message("System: ${CMAKE_SYSTEM_NAME}")
message("Project: ${PROJECT_NAME}")
message("Build: ${CMAKE_BUILD_TYPE}")
message("Flags: ${CMAKE_CXX_FLAGS}")
