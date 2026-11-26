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

set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

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

# Keep SDL_image focused on the common 2D project formats.
set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_INSTALL OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
set(SDLIMAGE_BMP ON CACHE BOOL "" FORCE)
set(SDLIMAGE_JPG ON CACHE BOOL "" FORCE)
set(SDLIMAGE_PNG ON CACHE BOOL "" FORCE)
set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_GIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_JXL OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_LBM OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PCX OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PNM OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_QOI OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_SVG OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TGA OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_XCF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_XPM OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_XV OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-3.2.4
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3_image)

add_library(ember_engine STATIC
    src/Engine/Core/Application.cpp
    src/Engine/Core/Time.cpp
    src/Engine/Resources/AssetDirectory.cpp
    src/Engine/Resources/AssetManager.cpp
    src/Engine/Resources/AssetMetadataStore.cpp
    src/Engine/Resources/TextureCache.cpp
    src/Engine/Scene/Scene.cpp
    src/Engine/Scene/SceneSerializer.cpp
    src/Engine/Renderer/Camera2D.cpp
    src/Engine/Platform/Input.cpp
    src/Engine/Renderer/Renderer2D.cpp
)

target_include_directories(ember_engine PUBLIC src)
target_link_libraries(ember_engine PUBLIC SDL3::SDL3 SDL3_image::SDL3_image PRIVATE nlohmann_json::nlohmann_json)

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

    add_executable(ember_scene_serializer_tests
        tests/SceneSerializerTests.cpp
        src/Engine/Scene/Scene.cpp
        src/Engine/Scene/SceneSerializer.cpp
    )
    target_include_directories(ember_scene_serializer_tests PRIVATE src)
    target_link_libraries(ember_scene_serializer_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember_scene_serializer_tests COMMAND ember_scene_serializer_tests)

    add_executable(ember_asset_metadata_tests
        tests/AssetMetadataTests.cpp
        src/Engine/Resources/AssetMetadataStore.cpp
    )
    target_include_directories(ember_asset_metadata_tests PRIVATE src)
    target_link_libraries(ember_asset_metadata_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember_asset_metadata_tests COMMAND ember_asset_metadata_tests)

    add_executable(ember_asset_manager_tests
        tests/AssetManagerTests.cpp
        src/Engine/Resources/AssetManager.cpp
        src/Engine/Resources/AssetDirectory.cpp
        src/Engine/Resources/TextureCache.cpp
    )
    target_include_directories(ember_asset_manager_tests PRIVATE src)
    add_test(NAME ember_asset_manager_tests COMMAND ember_asset_manager_tests)
endif()

if(MSVC)
    target_compile_options(ember_engine PRIVATE /W4 /permissive-)
    target_compile_options(ember_editor PRIVATE /W4 /permissive-)
    if(BUILD_TESTING)
        target_compile_options(ember_scene_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_texture_cache_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_asset_directory_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_asset_manager_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_asset_metadata_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_scene_serializer_tests PRIVATE /W4 /permissive-)
    endif()
else()
    target_compile_options(ember_engine PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ember_editor PRIVATE -Wall -Wextra -Wpedantic)
    if(BUILD_TESTING)
        target_compile_options(ember_scene_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_texture_cache_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_asset_directory_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_asset_manager_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_asset_metadata_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_scene_serializer_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

set_target_properties(ember_editor PROPERTIES
    OUTPUT_NAME "ember"
)
