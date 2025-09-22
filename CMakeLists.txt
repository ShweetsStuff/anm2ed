cmake_minimum_required(VERSION 3.15)
project(anm2ed CXX)

# Optional: auto-pick up vcpkg toolchain on Windows
if(WIN32 AND DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

find_package(OpenGL REQUIRED)

# Export compile_commands.json (for clangd, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    )
endif()

set(GLAD_SRC ${CMAKE_CURRENT_SOURCE_DIR}/include/glad/glad.cpp)

set(IMGUI_SRC
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/backends/imgui_impl_sdl3.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)

set(TINYXML2_SRC external/tinyxml2/tinyxml2.cpp)

file(GLOB PROJECT_SRC CONFIGURE_DEPENDS
    src/*.cpp
    src/*.h
)

add_executable(${PROJECT_NAME}
    ${GLAD_SRC}
    ${IMGUI_SRC}
    ${TINYXML2_SRC}
    ${PROJECT_SRC}
)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)

if(WIN32)
    enable_language(RC)
    target_sources(${PROJECT_NAME} PRIVATE Icon.rc)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

    set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc)
    target_link_options(${PROJECT_NAME} PRIVATE /STACK:0xffffff)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -O2 -Wall -Wextra -pedantic -fmax-errors=1
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
        target_compile_options(${PROJECT_NAME} PRIVATE -g)
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_DISABLE_OBSOLETE_FUNCTIONS IMGUI_DEBUG_PARANOID IMGUI_ENABLE_DOCKING)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

target_include_directories(${PROJECT_NAME} PRIVATE
    external
    external/imgui
    external/glm
    external/tinyxml2
    include
    include/glad
    src
)

target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL SDL3::SDL3-static)

message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Build: ${CMAKE_BUILD_TYPE}")