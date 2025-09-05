cmake_minimum_required(VERSION 3.15)

if(WIN32 AND DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(anm2ed CXX)

find_package(SDL3 REQUIRED)
find_package(OpenGL REQUIRED)

set(GLAD_SRC ${CMAKE_CURRENT_SOURCE_DIR}/include/glad/glad.cpp)

file(GLOB SOURCES
    ${GLAD_SRC}
    include/imgui/imgui.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/backends/imgui_impl_sdl3.cpp
    include/imgui/backends/imgui_impl_opengl3.cpp
    include/tinyxml2/tinyxml2.cpp
    src/*.cpp 
    src/*.h
)

if(NOT MSVC)
    set_source_files_properties(${GLAD_SRC} PROPERTIES COMPILE_FLAGS "-Wno-cast-function-type")
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

if(WIN32)
    enable_language(RC)
    target_sources(${PROJECT_NAME} PRIVATE assets/Icon.rc)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
target_include_directories(${PROJECT_NAME} PRIVATE include include/glad include/imgui include/tinyxml2 src)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /std:c++latest /EHsc)
    target_link_options(${PROJECT_NAME} PRIVATE /STACK:0xffffff)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -O2 -std=c++23 -Wall -Wextra -pedantic -fmax-errors=1)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE -DDEBUG -g)
    else()
        set(CMAKE_BUILD_TYPE "Release")
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL SDL3::SDL3)

message("System: ${CMAKE_SYSTEM_NAME}")
message("Project: ${PROJECT_NAME}")
message("Build: ${CMAKE_BUILD_TYPE}")