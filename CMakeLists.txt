cmake_minimum_required(VERSION 3.10)
project(anm2ed CXX)

# Gather project sources
file(GLOB SOURCES
    "${PROJECT_SOURCE_DIR}/include/imgui/*.cpp"
    "${PROJECT_SOURCE_DIR}/include/pugixml/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE include src)

set(CMAKE_CXX_FLAGS "-g -O2 -std=c++20 -Wall -Wextra -pedantic -Wno-unused-variable -Wno-unused-parameter -Wno-ignored-qualifiers -Wno-parentheses -Wno-unused-function -Wno-class-memaccess")

target_link_libraries(${PROJECT_NAME} PRIVATE m GL GLEW SDL3 SDL3_mixer)

message("System: ${CMAKE_SYSTEM_NAME}")
message("Project: ${PROJECT_NAME}")
message("Build: ${CMAKE_BUILD_TYPE}")
message("Flags: ${CMAKE_CXX_FLAGS}")
