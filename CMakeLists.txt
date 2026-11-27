cmake_minimum_required(VERSION 3.24)

project(EMBER
    VERSION 0.1.0
    DESCRIPTION "A lightweight, 2D-first game engine"
    LANGUAGES C CXX
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

# Box2D is the physics backend. Keep its own samples and tests disabled;
# EMBER owns the integration and its test coverage.
set(BOX2D_SAMPLES OFF CACHE BOOL "" FORCE)
set(BOX2D_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(BOX2D_DOCS OFF CACHE BOOL "" FORCE)
set(BOX2D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_PROFILE OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG v3.2.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(box2d)

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

# Dear ImGui is an editor-only dependency. It is deliberately excluded from
# ember_engine so runtime/game builds do not inherit editor UI code.
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9b
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(ember_engine STATIC
    src/Engine/Core/Application.cpp
    src/Engine/Core/Time.cpp
    src/Engine/Resources/AssetDirectory.cpp
    src/Engine/Resources/AssetManager.cpp
    src/Engine/Resources/AssetImportService.cpp
    src/Engine/Resources/AssetRegistry.cpp
    src/Engine/Resources/AssetMetadataStore.cpp
    src/Engine/Project/ProjectConfigurationStore.cpp
    src/Engine/Resources/TextureCache.cpp
    src/Engine/Scene/Scene.cpp
    src/Engine/Physics/PhysicsWorld.cpp
    src/Engine/Scene/SceneSerializer.cpp
    src/Engine/Renderer/Camera2D.cpp
    src/Engine/Platform/Input.cpp
    src/Engine/Platform/InputMap.cpp
    src/Engine/Renderer/Renderer2D.cpp
)

target_include_directories(ember_engine PUBLIC src)
target_link_libraries(ember_engine PUBLIC SDL3::SDL3 SDL3_image::SDL3_image nlohmann_json::nlohmann_json PRIVATE box2d::box2d)

add_library(ember_imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(ember_imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(ember_imgui PUBLIC SDL3::SDL3)

add_executable(ember_editor
    src/main.cpp
    src/Editor/EditorApplication.cpp
)
target_include_directories(ember_editor PRIVATE src)
target_link_libraries(ember_editor PRIVATE ember_engine ember_imgui SDL3::SDL3)

if(BUILD_TESTING)
    add_executable(ember_physics_components_tests
        tests/PhysicsComponentsTests.cpp
    )
    target_include_directories(ember_physics_components_tests PRIVATE src)
    target_link_libraries(ember_physics_components_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember_physics_components_tests COMMAND ember_physics_components_tests)

    add_executable(ember_physics_world_tests
        tests/PhysicsWorldTests.cpp
        src/Engine/Physics/PhysicsWorld.cpp
    )
    target_include_directories(ember_physics_world_tests PRIVATE src)
    target_link_libraries(ember_physics_world_tests PRIVATE box2d::box2d)
    add_test(NAME ember_physics_world_tests COMMAND ember_physics_world_tests)

    add_executable(ember_scene_physics_tests
        tests/ScenePhysicsTests.cpp
        src/Engine/Scene/Scene.cpp
        src/Engine/Physics/PhysicsWorld.cpp
    )
    target_include_directories(ember_scene_physics_tests PRIVATE src)
    target_link_libraries(ember_scene_physics_tests PRIVATE box2d::box2d)
    add_test(NAME ember_scene_physics_tests COMMAND ember_scene_physics_tests)

    add_executable(ember_input_map_tests
        tests/InputMapTests.cpp
        src/Engine/Platform/InputMap.cpp
    )
    target_include_directories(ember_input_map_tests PRIVATE src)
    # InputMap exposes SDL_Scancode in its public API. Linking SDL3 supplies
    # the SDL include path and definitions without initializing SDL at runtime.
    target_link_libraries(ember_input_map_tests PRIVATE SDL3::SDL3)
    add_test(NAME ember_input_map_tests COMMAND ember_input_map_tests)

    add_executable(ember_scene_tests
        tests/SceneTests.cpp
        src/Engine/Scene/Scene.cpp
        src/Engine/Physics/PhysicsWorld.cpp
    )
    target_include_directories(ember_scene_tests PRIVATE src)
    target_link_libraries(ember_scene_tests PRIVATE box2d::box2d)
    add_test(NAME ember_scene_tests COMMAND ember_scene_tests)

    add_executable(ember_component_tests
        tests/ComponentTests.cpp
        src/Engine/Scene/Scene.cpp
        src/Engine/Physics/PhysicsWorld.cpp
    )
    target_include_directories(ember_component_tests PRIVATE src)
    target_link_libraries(ember_component_tests PRIVATE box2d::box2d)
    add_test(NAME ember_component_tests COMMAND ember_component_tests)

    add_executable(ember_component_registry_tests
        tests/ComponentRegistryTests.cpp
    )
    target_include_directories(ember_component_registry_tests PRIVATE src)
    target_link_libraries(ember_component_registry_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember_component_registry_tests COMMAND ember_component_registry_tests)

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
        src/Engine/Physics/PhysicsWorld.cpp
        src/Engine/Scene/SceneSerializer.cpp
    )
    target_include_directories(ember_scene_serializer_tests PRIVATE src)
    target_link_libraries(ember_scene_serializer_tests PRIVATE nlohmann_json::nlohmann_json box2d::box2d)
    add_test(NAME ember_scene_serializer_tests COMMAND ember_scene_serializer_tests)

    add_executable(ember_asset_metadata_tests
        tests/AssetMetadataTests.cpp
        src/Engine/Resources/AssetMetadataStore.cpp
    )
    target_include_directories(ember_asset_metadata_tests PRIVATE src)
    target_link_libraries(ember_asset_metadata_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember_asset_metadata_tests COMMAND ember_asset_metadata_tests)

    add_executable(ember_asset_registry_tests
        tests/AssetRegistryTests.cpp
        src/Engine/Resources/AssetRegistry.cpp
        src/Engine/Resources/AssetMetadataStore.cpp
        src/Engine/Resources/AssetDirectory.cpp
    )
    target_include_directories(ember_asset_registry_tests PRIVATE src)
    target_link_libraries(ember_asset_registry_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember_asset_registry_tests COMMAND ember_asset_registry_tests)

    add_executable(ember_asset_manager_tests
        tests/AssetManagerTests.cpp
        src/Engine/Resources/AssetManager.cpp
        src/Engine/Resources/AssetRegistry.cpp
        src/Engine/Resources/AssetMetadataStore.cpp
        src/Engine/Resources/AssetDirectory.cpp
        src/Engine/Resources/TextureCache.cpp
    )
    target_include_directories(ember_asset_manager_tests PRIVATE src)
    target_link_libraries(ember_asset_manager_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember_asset_manager_tests COMMAND ember_asset_manager_tests)

    add_executable(ember_project_configuration_tests
        tests/ProjectConfigurationTests.cpp
        src/Engine/Project/ProjectConfigurationStore.cpp
    )
    target_include_directories(ember_project_configuration_tests PRIVATE src)
    target_link_libraries(ember_project_configuration_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember_project_configuration_tests COMMAND ember_project_configuration_tests)

    add_executable(ember_asset_import_service_tests
        tests/AssetImportServiceTests.cpp
        src/Engine/Resources/AssetImportService.cpp
        src/Engine/Resources/AssetRegistry.cpp
        src/Engine/Resources/AssetMetadataStore.cpp
        src/Engine/Resources/AssetDirectory.cpp
    )
    target_include_directories(ember_asset_import_service_tests PRIVATE src)
    target_link_libraries(ember_asset_import_service_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember_asset_import_service_tests COMMAND ember_asset_import_service_tests)
endif()

if(MSVC)
    target_compile_options(ember_engine PRIVATE /W4 /permissive-)
    target_compile_options(ember_editor PRIVATE /W4 /permissive-)
    target_compile_options(ember_imgui PRIVATE /W4 /permissive-)
    if(BUILD_TESTING)
        target_compile_options(ember_physics_world_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_scene_physics_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_physics_components_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_input_map_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_scene_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_component_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_component_registry_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_texture_cache_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_asset_directory_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_asset_manager_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_asset_registry_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_asset_metadata_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_scene_serializer_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_asset_import_service_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_project_configuration_tests PRIVATE /W4 /permissive-)
    endif()
else()
    target_compile_options(ember_engine PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ember_editor PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ember_imgui PRIVATE -Wall -Wextra -Wpedantic)
    if(BUILD_TESTING)
        target_compile_options(ember_physics_world_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_physics_components_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_input_map_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_scene_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_component_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_component_registry_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_texture_cache_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_asset_directory_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_asset_manager_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_asset_registry_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_asset_metadata_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_scene_serializer_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_asset_import_service_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_project_configuration_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

set_target_properties(ember_editor PROPERTIES
    OUTPUT_NAME "ember"
)
