cmake_minimum_required(VERSION 3.27)
project(anm2ed LANGUAGES C CXX)

if(WIN32 AND DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE
      "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "Vcpkg toolchain file")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(CMAKE_EXPORT_COMPILE_COMMANDS AND NOT WIN32)
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/compile_commands.json
                          ${CMAKE_SOURCE_DIR}/compile_commands.json)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug>:ProgramDatabase>")
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

macro(anm2ed_set_cache_bool name value)
  set(${name} ${value} CACHE BOOL "" FORCE)
endmacro()

anm2ed_set_cache_bool(SDL_STATIC ON)
anm2ed_set_cache_bool(SDL_SHARED OFF)
if(NOT WIN32)
  anm2ed_set_cache_bool(SDL_ALSA ON)
  anm2ed_set_cache_bool(SDL_ALSA_SHARED ON)
  anm2ed_set_cache_bool(SDL_JACK ON)
  anm2ed_set_cache_bool(SDL_JACK_SHARED ON)
  anm2ed_set_cache_bool(SDL_PIPEWIRE ON)
  anm2ed_set_cache_bool(SDL_PIPEWIRE_SHARED ON)
  anm2ed_set_cache_bool(SDL_SNDIO ON)
  anm2ed_set_cache_bool(SDL_SNDIO_SHARED ON)
endif()
anm2ed_set_cache_bool(SDL_HAPTIC OFF)
anm2ed_set_cache_bool(SDL_JOYSTICK OFF)
anm2ed_set_cache_bool(SDL_SENSOR OFF)
anm2ed_set_cache_bool(SDL_HIDAPI OFF)
anm2ed_set_cache_bool(SDL_CAMERA OFF)
anm2ed_set_cache_bool(SDL_TRAY OFF)
anm2ed_set_cache_bool(SDL_VULKAN OFF)
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)

anm2ed_set_cache_bool(SDLMIXER_DEPS_SHARED OFF)
anm2ed_set_cache_bool(SDLMIXER_AIFF OFF)
anm2ed_set_cache_bool(SDLMIXER_AU OFF)
anm2ed_set_cache_bool(SDLMIXER_FLAC_LIBFLAC OFF)
anm2ed_set_cache_bool(SDLMIXER_FLAC_DRFLAC OFF)
anm2ed_set_cache_bool(SDLMIXER_GME OFF)
anm2ed_set_cache_bool(SDLMIXER_MOD_XMP OFF)
anm2ed_set_cache_bool(SDLMIXER_MP3_DRMP3 OFF)
anm2ed_set_cache_bool(SDLMIXER_MP3_MPG123 OFF)
anm2ed_set_cache_bool(SDLMIXER_MIDI_FLUIDSYNTH OFF)
anm2ed_set_cache_bool(SDLMIXER_MIDI_TIMIDITY OFF)
anm2ed_set_cache_bool(SDLMIXER_OPUS ON)
anm2ed_set_cache_bool(SDLMIXER_VOC OFF)
anm2ed_set_cache_bool(SDLMIXER_VORBIS_STB ON)
anm2ed_set_cache_bool(SDLMIXER_VORBIS_VORBISFILE OFF)
anm2ed_set_cache_bool(SDLMIXER_VORBIS_TREMOR OFF)
anm2ed_set_cache_bool(SDLMIXER_WAVE ON)
anm2ed_set_cache_bool(SDLMIXER_WAVPACK OFF)
anm2ed_set_cache_bool(SDLMIXER_TEST OFF)
anm2ed_set_cache_bool(SDLMIXER_INSTALL OFF)
add_subdirectory(external/SDL_mixer EXCLUDE_FROM_ALL)

add_subdirectory(external/lunasvg)

set(GLAD_SRC ${CMAKE_CURRENT_SOURCE_DIR}/include/glad/glad.cpp)

set(IMGUI_SRC
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/backends/imgui_impl_sdl3.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp)

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
     src/imgui/wizard/*.cpp
     src/imgui/wizard/*.h
     src/util/*.cpp
     src/util/*.h
     src/window/*.cpp
     src/window/*.h
     src/*.cpp
     src/*.h)

add_executable(${PROJECT_NAME} ${GLAD_SRC} ${IMGUI_SRC} ${TINYXML2_SRC} ${PROJECT_SRC})

target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_DISABLE_OBSOLETE_FUNCTIONS IMGUI_DEBUG_PARANOID
                                               IMGUI_ENABLE_DOCKING)

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE include
          include/glad
          external
          external/imgui
          external/glm
          external/tinyxml2
          external/lunasvg
          external/SDL
          external/SDL_mixer
          external/SDL_mixer/include
          src
          src/imgui
          src/resource
          src/util)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
endif()

if(WIN32)
  # Keep Release as a GUI app (no console window).
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
      target_link_options(${PROJECT_NAME} PRIVATE /SUBSYSTEM:WINDOWS)
    else()
      target_link_options(${PROJECT_NAME} PRIVATE -mwindows)
    endif()
  endif()
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -O0 -pg)
  else()
    set(CMAKE_BUILD_TYPE "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE -Os)
  endif()
  target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()

if(WIN32)
  enable_language(RC)
  target_sources(${PROJECT_NAME} PRIVATE anm2ed.rc)
endif()

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/std:c++latest>")
  target_compile_options(${PROJECT_NAME} PRIVATE /EHsc)
  target_link_options(${PROJECT_NAME} PRIVATE /STACK:0xffffff)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE /Zi)
    target_link_options(${PROJECT_NAME} PRIVATE /DEBUG)
  endif()
endif()

if(WIN32)
  set(OPENGL_LIB opengl32)
  set(PLATFORM_LIBS
      ws2_32
      bcrypt
      imm32
      version
      winmm
      shell32
      ole32
      advapi32
      gdi32
      user32
      setupapi
      legacy_stdio_definitions)
  if(TARGET SDL3::SDL3main)
    set(SDL_MAIN_TARGET SDL3::SDL3main)
  else()
    set(SDL_MAIN_TARGET)
  endif()
else()
  set(OPENGL_LIB GL)
  set(PLATFORM_LIBS)
  set(SDL_MAIN_TARGET)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_LIB} SDL3-static ${SDL_MAIN_TARGET}
                                            SDL3_mixer::SDL3_mixer lunasvg ${PLATFORM_LIBS})

message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")

get_target_property(PROJECT_COMPILE_OPTIONS ${PROJECT_NAME} COMPILE_OPTIONS)
if(NOT PROJECT_COMPILE_OPTIONS)
  set(PROJECT_COMPILE_OPTIONS "<none>")
endif()

string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)
set(EFFECTIVE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
if(BUILD_TYPE_UPPER)
  set(CONFIG_FLAGS_VAR "CMAKE_CXX_FLAGS_${BUILD_TYPE_UPPER}")
  if(DEFINED ${CONFIG_FLAGS_VAR})
    string(APPEND EFFECTIVE_CXX_FLAGS " ${${CONFIG_FLAGS_VAR}}")
  endif()
endif()
string(STRIP "${EFFECTIVE_CXX_FLAGS}" EFFECTIVE_CXX_FLAGS)
if(EFFECTIVE_CXX_FLAGS STREQUAL "")
  set(EFFECTIVE_CXX_FLAGS "<none>")
endif()

message(STATUS "Compiler Flags: ${EFFECTIVE_CXX_FLAGS}")
message(STATUS "Target Compile Options: ${PROJECT_COMPILE_OPTIONS}")
message(STATUS "Build: ${CMAKE_BUILD_TYPE}")
