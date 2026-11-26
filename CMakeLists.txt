cmake_minimum_required(VERSION 3.24)

project(EMBER
    VERSION 0.1.0
    DESCRIPTION "A lightweight, 2D-first game engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.10
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3)

add_library(ember_engine STATIC
    src/Engine/Core/Application.cpp
    src/Engine/Core/Time.cpp
    src/Engine/Platform/Input.cpp
    src/Engine/Renderer/Renderer2D.cpp
)

target_include_directories(ember_engine PUBLIC src)
target_link_libraries(ember_engine PUBLIC SDL3::SDL3)

add_executable(ember_editor
    src/main.cpp
)

target_link_libraries(ember_editor PRIVATE ember_engine)

if(MSVC)
    target_compile_options(ember_engine PRIVATE /W4 /permissive-)
    target_compile_options(ember_editor PRIVATE /W4 /permissive-)
else()
    target_compile_options(ember_engine PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ember_editor PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(ember_editor PROPERTIES
    OUTPUT_NAME "ember"
)
