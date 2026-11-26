cmake_minimum_required(VERSION 3.24)

project(EMBER
    VERSION 0.1.0
    DESCRIPTION "A lightweight, 2D-first game engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
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
    src/Engine/Resources/AssetDirectory.cpp
    src/Engine/Resources/TextureCache.cpp
    src/Engine/Scene/Scene.cpp
    src/Engine/Renderer/Camera2D.cpp
    src/Engine/Platform/Input.cpp
    src/Engine/Renderer/Renderer2D.cpp
)

target_include_directories(ember_engine PUBLIC src)
target_link_libraries(ember_engine PUBLIC SDL3::SDL3)

add_executable(ember_editor
    src/main.cpp
)

target_link_libraries(ember_editor PRIVATE ember_engine)

if(BUILD_TESTING)
    add_executable(ember_scene_tests
        tests/SceneTests.cpp
        src/Engine/Scene/Scene.cpp
    )
    target_include_directories(ember_scene_tests PRIVATE src)
    add_test(NAME ember_scene_tests COMMAND ember_scene_tests)

    add_executable(ember_texture_cache_tests
        tests/TextureCacheTests.cpp
        src/Engine/Resources/TextureCache.cpp
    )
    target_include_directories(ember_texture_cache_tests PRIVATE src)
    add_test(NAME ember_texture_cache_tests COMMAND ember_texture_cache_tests)

    add_executable(ember_asset_directory_tests
        tests/AssetDirectoryTests.cpp
        src/Engine/Resources/AssetDirectory.cpp
    )
    target_include_directories(ember_asset_directory_tests PRIVATE src)
    add_test(NAME ember_asset_directory_tests COMMAND ember_asset_directory_tests)
endif()

if(MSVC)
    target_compile_options(ember_engine PRIVATE /W4 /permissive-)
    target_compile_options(ember_editor PRIVATE /W4 /permissive-)
    if(BUILD_TESTING)
        target_compile_options(ember_scene_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_texture_cache_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_asset_directory_tests PRIVATE /W4 /permissive-)
    endif()
else()
    target_compile_options(ember_engine PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ember_editor PRIVATE -Wall -Wextra -Wpedantic)
    if(BUILD_TESTING)
        target_compile_options(ember_scene_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_texture_cache_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_asset_directory_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

set_target_properties(ember_editor PROPERTIES
    OUTPUT_NAME "ember"
)
