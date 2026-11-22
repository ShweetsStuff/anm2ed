cmake_minimum_required(VERSION 3.27)
project(anm2ed LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Two builds: Release (the default) and Debug. Set before the libraries are added, so they're optimized too.
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_BUILD_TYPE Release)
endif()

# Linux release builds: -O2, unused code and data dropped, and the symbols stripped.
# Link-time optimization is left off: it made the binary bigger and every relink take seconds longer.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_options("$<$<CONFIG:Release>:-O2;-ffunction-sections;-fdata-sections>")
    add_link_options("$<$<CONFIG:Release>:-Wl,--gc-sections;-s>")
endif()

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/std:c++latest>" /EHsc)
endif()

# SDL: static, with only video (OpenGL), audio, events, clipboard and file dialogs.
# Its audio plays WAV and, through stb_vorbis, Ogg Vorbis.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
foreach(OPTION TEST_LIBRARY EXAMPLES RENDER GPU VULKAN OPENGLES CAMERA HAPTIC JOYSTICK HIDAPI SENSOR POWER TRAY
        OFFSCREEN KMSDRM DUMMYVIDEO DISKAUDIO DUMMYAUDIO OSS JACK SNDIO LIBURING)
    set(SDL_${OPTION} OFF CACHE BOOL "" FORCE)
endforeach()
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)

set(LUNASVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/lunasvg EXCLUDE_FROM_ALL)

# simdjson: its single-file build, without exceptions, with only the AVX2 parser and the portable one.
add_library(simdjson STATIC external/simdjson/singleheader/simdjson.cpp)
target_compile_definitions(simdjson PUBLIC SIMDJSON_EXCEPTIONS=0 SIMDJSON_IMPLEMENTATION_ICELAKE=0
                           SIMDJSON_IMPLEMENTATION_WESTMERE=0)

# Everything under resources/ is compiled in, through one generated C file (see embed.cmake).
file(GLOB_RECURSE RESOURCES CONFIGURE_DEPENDS resources/*)
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/embed/embed.c ${CMAKE_BINARY_DIR}/embed/embed.h
    COMMAND ${CMAKE_COMMAND} -DRESOURCES=${CMAKE_SOURCE_DIR}/resources -DOUTPUT=${CMAKE_BINARY_DIR}/embed
            -P ${CMAKE_SOURCE_DIR}/embed.cmake
    DEPENDS ${RESOURCES} ${CMAKE_SOURCE_DIR}/embed.cmake)

# The core: the model, edits, undo and file formats, with no window or OpenGL. The tests link it alone.
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS src/edit/*.cpp src/model/*.cpp)
foreach(FILE log image audio_data asset_store file path math_util timestamp working_directory pack ui/multiselect
        snapshots playback selection)
    list(APPEND CORE_SOURCES src/${FILE}.cpp)
endforeach()
add_library(anm2ed_core STATIC ${CORE_SOURCES} external/pugixml/src/pugixml.cpp external/imgui/imgui.cpp
            external/imgui/imgui_draw.cpp external/imgui/imgui_widgets.cpp external/imgui/imgui_tables.cpp
            ${CMAKE_BINARY_DIR}/embed/embed.c)
target_include_directories(anm2ed_core PUBLIC include external external/imgui external/glm external/stb src
                           ${CMAKE_BINARY_DIR}/embed)
target_link_libraries(anm2ed_core PUBLIC lunasvg simdjson SDL3::Headers)
target_compile_definitions(anm2ed_core PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS
                           $<$<CONFIG:Debug>:IMGUI_DEBUG_PARANOID> $<$<BOOL:${WIN32}>:NOMINMAX>)

# The editor: the core with its window, OpenGL and interface. The render tests link it too.
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS src/*.cpp)
list(REMOVE_ITEM APP_SOURCES ${CORE_SOURCES} ${CMAKE_SOURCE_DIR}/src/main.cpp)
set_source_files_properties(external/stb/stb_vorbis.c PROPERTIES COMPILE_OPTIONS -w)
add_library(anm2ed_app STATIC ${APP_SOURCES} external/stb/stb_vorbis.c include/glad/glad.cpp external/imgui/backends/imgui_impl_sdl3.cpp
            external/imgui/backends/imgui_impl_opengl3.cpp)
if(WIN32)
    set(PLATFORM_LIBRARIES opengl32 ws2_32 bcrypt imm32 version winmm shell32 ole32 advapi32 gdi32 user32 setupapi
        $<$<BOOL:${MSVC}>:legacy_stdio_definitions>)
else()
    set(PLATFORM_LIBRARIES GL)
endif()
target_link_libraries(anm2ed_app PUBLIC anm2ed_core SDL3::SDL3-static ${PLATFORM_LIBRARIES})

add_executable(anm2ed src/main.cpp $<$<BOOL:${WIN32}>:anm2ed.rc>)
target_link_libraries(anm2ed PRIVATE anm2ed_app)
set_target_properties(anm2ed PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if(MSVC)
    target_link_options(anm2ed PRIVATE /STACK:0xffffff $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>)
elseif(WIN32)
    target_link_options(anm2ed PRIVATE $<$<CONFIG:Release>:-mwindows>)
endif()

if(NOT MSVC)
    foreach(TARGET anm2ed_core anm2ed_app anm2ed)
        target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic -Wno-missing-field-initializers)
    endforeach()
endif()

option(ANM2ED_TESTS "Build the tests" ON)
if(ANM2ED_TESTS)
    enable_testing()
    set(TEST_DIRECTORY ${CMAKE_SOURCE_DIR}/tests)

    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
    add_executable(anm2ed_tests ${TEST_SOURCES})
    target_link_libraries(anm2ed_tests PRIVATE anm2ed_core)
    target_include_directories(anm2ed_tests PRIVATE external/doctest/doctest)
    target_compile_definitions(anm2ed_tests PRIVATE ANM2ED_TEST_DIR="${TEST_DIRECTORY}")
    add_test(NAME anm2ed_tests COMMAND anm2ed_tests WORKING_DIRECTORY ${TEST_DIRECTORY})

    # Drawing needs OpenGL (skipped without a display). Renders land in build/renders for a look.
    file(GLOB RENDER_TEST_SOURCES CONFIGURE_DEPENDS tests/render/*.cpp)
    add_executable(anm2ed_render_tests tests/main.cpp ${RENDER_TEST_SOURCES})
    target_link_libraries(anm2ed_render_tests PRIVATE anm2ed_app)
    target_include_directories(anm2ed_render_tests PRIVATE external/doctest/doctest tests)
    target_compile_definitions(anm2ed_render_tests PRIVATE ANM2ED_TEST_DIR="${TEST_DIRECTORY}"
                               ANM2ED_RENDER_DIR="${CMAKE_BINARY_DIR}/renders")
    add_test(NAME anm2ed_render_tests COMMAND anm2ed_render_tests WORKING_DIRECTORY ${TEST_DIRECTORY})
    add_test(NAME anm2ed_cli_info COMMAND anm2ed info ${TEST_DIRECTORY}/render/spin.anm2)
endif()
