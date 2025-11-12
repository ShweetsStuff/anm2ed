cmake_minimum_required(VERSION 3.27)
project(anm2ed LANGUAGES C CXX)

if (WIN32 AND DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (CMAKE_EXPORT_COMPILE_COMMANDS AND NOT WIN32)
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
    )
endif ()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
if (NOT WIN32)
    set(SDL_ALSA ON CACHE BOOL "" FORCE)
    set(SDL_ALSA_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_JACK ON CACHE BOOL "" FORCE)
    set(SDL_JACK_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_PIPEWIRE ON CACHE BOOL "" FORCE)
    set(SDL_PIPEWIRE_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_SNDIO ON CACHE BOOL "" FORCE)
    set(SDL_SNDIO_SHARED ON CACHE BOOL "" FORCE)
endif ()
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_TRAY OFF CACHE BOOL "" FORCE)
set(SDL_VULKAN OFF CACHE BOOL "" FORCE)
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)

set(SDLMIXER_DEPS_SHARED OFF CACHE BOOL "" FORCE)
set(SDLMIXER_FLAC_LIBFLAC OFF CACHE BOOL "" FORCE)
set(SDLMIXER_GME OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MOD_XMP OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MP3_MPG123 OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MIDI_FLUIDSYNTH OFF CACHE BOOL "" FORCE)
set(SDLMIXER_OPUS ON CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_VORBISFILE OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_TREMOR OFF CACHE BOOL "" FORCE)
set(SDLMIXER_WAVPACK OFF CACHE BOOL "" FORCE)
set(SDLMIXER_TEST OFF CACHE BOOL "" FORCE)
set(SDLMIXER_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/SDL_mixer EXCLUDE_FROM_ALL)

add_subdirectory(external/lunasvg)


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
        src/anm2/*.cpp
        src/anm2/*.h
        src/resource/*.cpp
        src/resource/*.h
        src/imgui/*.cpp
        src/imgui/*.h
        src/imgui/window/*.cpp
        src/imgui/window/*.h
        src/util/*.cpp
        src/util/*.h
        src/window/*.cpp
        src/window/*.h
        src/*.cpp
        src/*.h
)

add_executable(${PROJECT_NAME}
        ${GLAD_SRC}
        ${IMGUI_SRC}
        ${TINYXML2_SRC}
        ${PROJECT_SRC}
)

if (WIN32)
    enable_language(RC)
    target_sources(${PROJECT_NAME} PRIVATE Icon.rc)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

    set_property(TARGET ${PROJECT_NAME} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc)
    target_link_options(${PROJECT_NAME} PRIVATE /STACK:0xffffff)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE
            -Wall -Wextra -pedantic
    )
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
        target_compile_options(${PROJECT_NAME} PRIVATE -O0 -pg)
    else ()
        set(CMAKE_BUILD_TYPE "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE -Os)
    endif ()

    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif ()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        IMGUI_DISABLE_OBSOLETE_FUNCTIONS
        IMGUI_DEBUG_PARANOID
        IMGUI_ENABLE_DOCKING
)


if (MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE restrict=__restrict)
else ()
    target_compile_definitions(${PROJECT_NAME} PRIVATE restrict=__restrict__)
endif ()

target_include_directories(${PROJECT_NAME} PRIVATE
        external
        external/imgui
        external/glm
        external/tinyxml2
        external/lunasvg
        external/SDL
        external/SDL_mixer
        include
        include/glad
        src
        src/imgui
        src/resource
        src/util
)

target_link_libraries(${PROJECT_NAME} PRIVATE GL SDL3-static SDL3_mixer::SDL3_mixer lunasvg)

message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")

get_target_property(PROJECT_COMPILE_OPTIONS ${PROJECT_NAME} COMPILE_OPTIONS)
if (NOT PROJECT_COMPILE_OPTIONS)
    set(PROJECT_COMPILE_OPTIONS "<none>")
endif ()

string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)
set(EFFECTIVE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
if (BUILD_TYPE_UPPER)
    set(CONFIG_FLAGS_VAR "CMAKE_CXX_FLAGS_${BUILD_TYPE_UPPER}")
    if (DEFINED ${CONFIG_FLAGS_VAR})
        string(APPEND EFFECTIVE_CXX_FLAGS " ${${CONFIG_FLAGS_VAR}}")
    endif ()
endif ()
string(STRIP "${EFFECTIVE_CXX_FLAGS}" EFFECTIVE_CXX_FLAGS)
if (EFFECTIVE_CXX_FLAGS STREQUAL "")
    set(EFFECTIVE_CXX_FLAGS "<none>")
endif ()

message(STATUS "Compiler Flags: ${EFFECTIVE_CXX_FLAGS}")
message(STATUS "Target Compile Options: ${PROJECT_COMPILE_OPTIONS}")
message(STATUS "Build: ${CMAKE_BUILD_TYPE}")
