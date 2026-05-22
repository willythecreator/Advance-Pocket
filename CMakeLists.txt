cmake_minimum_required(VERSION 3.20)
project(AdvancePocket VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

set(SOURCES
    src/main.cpp
    src/window.cpp
    src/renderer.cpp
    src/emulator.cpp
    src/gba/cpu.cpp
    src/gba/memory.cpp
    src/gba/ppu.cpp
)

add_executable(AdvancePocket ${SOURCES})

target_include_directories(AdvancePocket PRIVATE
    include
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)

target_link_libraries(AdvancePocket PRIVATE
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    dl
)

target_compile_options(AdvancePocket PRIVATE
    -Wall -Wextra -Wpedantic
    -O2
    $<$<CONFIG:Debug>:-g -O0 -DDEBUG>
)

message(STATUS "AdvancePocket ${PROJECT_VERSION} — SDL2 found, OpenGL found")