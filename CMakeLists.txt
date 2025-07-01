cmake_minimum_required(VERSION 3.15)
if(WIN32 AND DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(anm2ed CXX)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

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
	"${PROJECT_SOURCE_DIR}/src/*.h"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE include include/imgui include/tinyxml2 src)
if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "-g -O2 -std=c++23 -Wall -Wextra -pedantic -fmax-errors=1")
else()
    set(CMAKE_CXX_FLAGS "/std:c++latest /EHsc") # /std:c++latest is required to make MSVC grant access to all the latest C++ stuff (C++23 is listed as preview even on dev previews of MSVC....)
endif()

if(NOT MSVC)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL GLEW::GLEW SDL3::SDL3)

message("System: ${CMAKE_SYSTEM_NAME}")
message("Project: ${PROJECT_NAME}")
message("Build: ${CMAKE_BUILD_TYPE}")
message("Flags: ${CMAKE_CXX_FLAGS}")
