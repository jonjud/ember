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
find_package(Threads REQUIRED)

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


# Lua 5.5.1 is compiled as a private static dependency. Only approved standard
# libraries are compiled into the backend; filesystem/process libraries are omitted.
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG v5.5.1
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(lua)
if(NOT lua_POPULATED)
    FetchContent_Populate(lua)
endif()

add_library(ember_lua STATIC
    ${lua_SOURCE_DIR}/lapi.c
    ${lua_SOURCE_DIR}/lcode.c
    ${lua_SOURCE_DIR}/lctype.c
    ${lua_SOURCE_DIR}/ldebug.c
    ${lua_SOURCE_DIR}/ldo.c
    ${lua_SOURCE_DIR}/ldump.c
    ${lua_SOURCE_DIR}/lfunc.c
    ${lua_SOURCE_DIR}/lgc.c
    ${lua_SOURCE_DIR}/llex.c
    ${lua_SOURCE_DIR}/lmem.c
    ${lua_SOURCE_DIR}/lobject.c
    ${lua_SOURCE_DIR}/lopcodes.c
    ${lua_SOURCE_DIR}/lparser.c
    ${lua_SOURCE_DIR}/lstate.c
    ${lua_SOURCE_DIR}/lstring.c
    ${lua_SOURCE_DIR}/ltable.c
    ${lua_SOURCE_DIR}/ltm.c
    ${lua_SOURCE_DIR}/lundump.c
    ${lua_SOURCE_DIR}/lvm.c
    ${lua_SOURCE_DIR}/lzio.c
    ${lua_SOURCE_DIR}/lauxlib.c
    ${lua_SOURCE_DIR}/lbaselib.c
    ${lua_SOURCE_DIR}/lmathlib.c
    ${lua_SOURCE_DIR}/lstrlib.c
    ${lua_SOURCE_DIR}/ltablib.c
    ${lua_SOURCE_DIR}/lutf8lib.c
)
target_include_directories(ember_lua PUBLIC ${lua_SOURCE_DIR})
set_target_properties(ember_lua PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)

# miniaudio is compiled once behind EMBER's engine-owned audio facade.
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.26
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(miniaudio)
if(NOT miniaudio_POPULATED)
    FetchContent_Populate(miniaudio)
endif()

add_library(ember_miniaudio STATIC ${miniaudio_SOURCE_DIR}/miniaudio.c)
target_include_directories(ember_miniaudio PUBLIC ${miniaudio_SOURCE_DIR})

add_library(ember_engine STATIC
    src/Engine/Core/Logger.cpp
    src/Engine/Core/FileSystem.cpp
    src/Engine/Scripting/LuaScriptRuntime.cpp
    src/Engine/Core/Application.cpp
    src/Engine/Core/Time.cpp
    src/Engine/Audio/AudioEngine.cpp
    src/Engine/Resources/AssetDirectory.cpp
    src/Engine/Resources/AssetManager.cpp
    src/Engine/Resources/AssetImportService.cpp
    src/Engine/Resources/AssetRegistry.cpp
    src/Engine/Resources/AssetMetadataStore.cpp
    src/Engine/Project/ProjectConfigurationStore.cpp
    src/Engine/Project/ProjectExporter.cpp
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
target_link_libraries(ember_engine PUBLIC SDL3::SDL3 SDL3_image::SDL3_image nlohmann_json::nlohmann_json Threads::Threads PRIVATE box2d::box2d ember_miniaudio ember_lua)

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

# Standalone game runtime: intentionally excludes Dear ImGui and all Editor code.
add_executable(ember_runtime
    src/Runtime/main.cpp
    src/Runtime/RuntimeApplication.cpp
)
target_include_directories(ember_runtime PRIVATE src)
target_link_libraries(ember_runtime PRIVATE ember_engine SDL3::SDL3)

# Stage shared runtime dependencies beside the standalone executable. The
# exporter collects DLLs from this directory into the final game package.
add_custom_command(TARGET ember_runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:ember_runtime>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3_image::SDL3_image>
        $<TARGET_FILE_DIR:ember_runtime>
    VERBATIM
)

if(BUILD_TESTING)
    add_executable(ember_logging_tests
        tests/LoggingTests.cpp
        src/Engine/Core/Logger.cpp
    )
    target_include_directories(ember_logging_tests PRIVATE src)
    target_link_libraries(ember_logging_tests PRIVATE Threads::Threads)
    add_test(NAME ember_logging_history_tests COMMAND ember_logging_tests history)
    add_test(NAME ember_logging_recording_sink_tests COMMAND ember_logging_tests recording)
    add_test(NAME ember_logging_file_sink_tests COMMAND ember_logging_tests file)
    add_test(NAME ember_logging_file_rotation_tests COMMAND ember_logging_tests rotation)
    add_test(NAME ember_logging_zero_capacity_sink_tests COMMAND ember_logging_tests zero-capacity)
    set_tests_properties(ember_logging_recording_sink_tests ember_logging_file_sink_tests ember_logging_file_rotation_tests ember_logging_zero_capacity_sink_tests PROPERTIES TIMEOUT 30)
    add_test(NAME ember_logging_concurrency_tests COMMAND ember_logging_tests concurrency)

    add_executable(ember_audio_engine_tests
        tests/AudioEngineTests.cpp
        src/Engine/Audio/AudioEngine.cpp
        src/Engine/Core/FileSystem.cpp
    )
    target_include_directories(ember_audio_engine_tests PRIVATE src ${miniaudio_SOURCE_DIR})
    target_link_libraries(ember_audio_engine_tests PRIVATE ember_miniaudio)
    add_test(NAME ember_audio_engine_tests COMMAND ember_audio_engine_tests)

    add_executable(ember_audio_scene_tests
        tests/AudioSceneTests.cpp
        src/Engine/Audio/AudioEngine.cpp
        src/Engine/Resources/AssetDirectory.cpp
        src/Engine/Resources/AssetMetadataStore.cpp
        src/Engine/Core/FileSystem.cpp
        src/Engine/Resources/AssetRegistry.cpp
        src/Engine/Scene/Scene.cpp
        src/Engine/Physics/PhysicsWorld.cpp
    )
    target_include_directories(ember_audio_scene_tests PRIVATE src ${miniaudio_SOURCE_DIR})
    target_link_libraries(ember_audio_scene_tests PRIVATE
        box2d::box2d ember_miniaudio nlohmann_json::nlohmann_json)
    add_test(NAME ember_audio_scene_tests COMMAND ember_audio_scene_tests)

    add_executable(ember_ui_components_tests
        tests/UIComponentsTests.cpp
    )
    target_include_directories(ember_ui_components_tests PRIVATE src)
    target_link_libraries(ember_ui_components_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember_ui_components_tests COMMAND ember_ui_components_tests)

    add_executable(ember_ui_input_tests
        tests/UIInputRouterTests.cpp
        src/Engine/Scene/Scene.cpp
        src/Engine/Physics/PhysicsWorld.cpp
    )
    target_include_directories(ember_ui_input_tests PRIVATE src)
    target_link_libraries(ember_ui_input_tests PRIVATE nlohmann_json::nlohmann_json box2d::box2d)
    add_test(NAME ember_ui_input_tests COMMAND ember_ui_input_tests)

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

    add_executable(ember_sprite_animation_tests
        tests/SpriteAnimationTests.cpp
        src/Engine/Scene/Scene.cpp
        src/Engine/Physics/PhysicsWorld.cpp
    )
    target_include_directories(ember_sprite_animation_tests PRIVATE src)
    target_link_libraries(ember_sprite_animation_tests PRIVATE box2d::box2d)
    add_test(NAME ember_sprite_animation_tests COMMAND ember_sprite_animation_tests)

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
        src/Engine/Platform/Input.cpp
        src/Engine/Platform/InputMap.cpp
    )
    target_include_directories(ember_component_tests PRIVATE src)
    target_link_libraries(ember_component_tests PRIVATE box2d::box2d SDL3::SDL3)
    # SDL3 is a shared dependency of Input.cpp; place the runtime DLL beside
    # the test executable so Windows can load it before entering main().
    add_custom_command(TARGET ember_component_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:ember_component_tests>
    )
    add_test(NAME ember_component_tests COMMAND ember_component_tests)
    set_tests_properties(ember_component_tests PROPERTIES TIMEOUT 60)

    add_executable(ember_script_component_tests
        tests/ScriptComponentTests.cpp
    )
    target_include_directories(ember_script_component_tests PRIVATE src)
    target_link_libraries(ember_script_component_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember_script_component_tests COMMAND ember_script_component_tests)

    add_executable(ember_script_runtime_scene_tests
        tests/ScriptRuntimeSceneTests.cpp
    )
    target_include_directories(ember_script_runtime_scene_tests PRIVATE src)
    target_link_libraries(ember_script_runtime_scene_tests PRIVATE ember_engine)
    add_custom_command(TARGET ember_script_runtime_scene_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:ember_script_runtime_scene_tests>
    )
    add_test(NAME ember_script_runtime_scene_tests COMMAND ember_script_runtime_scene_tests)

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
        src/Engine/Core/FileSystem.cpp
    )
    target_include_directories(ember_asset_metadata_tests PRIVATE src)
    target_link_libraries(ember_asset_metadata_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember_asset_metadata_tests COMMAND ember_asset_metadata_tests)

    add_executable(ember_asset_registry_tests
        tests/AssetRegistryTests.cpp
        src/Engine/Resources/AssetRegistry.cpp
        src/Engine/Resources/AssetMetadataStore.cpp
        src/Engine/Resources/AssetDirectory.cpp
        src/Engine/Core/FileSystem.cpp
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
        src/Engine/Core/FileSystem.cpp
    )
    target_include_directories(ember_asset_manager_tests PRIVATE src)
    target_link_libraries(ember_asset_manager_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember_asset_manager_tests COMMAND ember_asset_manager_tests)

    add_executable(ember_project_exporter_tests
        tests/ProjectExporterTests.cpp
        src/Engine/Project/ProjectExporter.cpp
        src/Engine/Project/ProjectConfigurationStore.cpp
        src/Engine/Resources/AssetRegistry.cpp
        src/Engine/Resources/AssetMetadataStore.cpp
        src/Engine/Resources/AssetDirectory.cpp
        src/Engine/Core/FileSystem.cpp
    )
    target_include_directories(ember_project_exporter_tests PRIVATE src)
    target_link_libraries(ember_project_exporter_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember_project_exporter_tests COMMAND ember_project_exporter_tests)

    add_executable(ember_project_configuration_tests
        tests/ProjectConfigurationTests.cpp
        src/Engine/Project/ProjectConfigurationStore.cpp
        src/Engine/Core/FileSystem.cpp
    )
    target_include_directories(ember_project_configuration_tests PRIVATE src)
    target_link_libraries(ember_project_configuration_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember_project_configuration_tests COMMAND ember_project_configuration_tests)


    add_executable(ember_lua_script_runtime_tests
        tests/LuaScriptRuntimeTests.cpp
    )
    target_include_directories(ember_lua_script_runtime_tests PRIVATE src)
    target_link_libraries(ember_lua_script_runtime_tests PRIVATE ember_engine SDL3::SDL3)
    add_custom_command(TARGET ember_lua_script_runtime_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:ember_lua_script_runtime_tests>
    )
    add_test(NAME ember_lua_script_runtime_tests COMMAND ember_lua_script_runtime_tests)

    add_executable(ember_asset_import_service_tests
        tests/AssetImportServiceTests.cpp
        src/Engine/Resources/AssetImportService.cpp
        src/Engine/Resources/AssetRegistry.cpp
        src/Engine/Resources/AssetMetadataStore.cpp
        src/Engine/Resources/AssetDirectory.cpp
        src/Engine/Core/FileSystem.cpp
    )
    target_include_directories(ember_asset_import_service_tests PRIVATE src)
    target_link_libraries(ember_asset_import_service_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember_asset_import_service_tests COMMAND ember_asset_import_service_tests)
endif()

# Scene-based test executables compile Scene.cpp directly rather than linking
# ember_engine. Supply the AudioEngine implementation/backend now that Scene
# owns transient AudioSoundHandles during its runtime lifecycle.
if(BUILD_TESTING)
    foreach(scene_target IN ITEMS
        ember_audio_scene_tests
        ember_scene_physics_tests
        ember_sprite_animation_tests
        ember_scene_tests
        ember_component_tests
        ember_scene_serializer_tests
        ember_ui_input_tests
    )
        target_sources(${scene_target} PRIVATE
            src/Engine/Core/Logger.cpp
            src/Engine/Audio/AudioEngine.cpp
            src/Engine/Core/FileSystem.cpp
            src/Engine/Resources/AssetDirectory.cpp
            src/Engine/Resources/AssetMetadataStore.cpp
            src/Engine/Resources/AssetRegistry.cpp
        )
        target_include_directories(${scene_target} PRIVATE ${miniaudio_SOURCE_DIR})
        target_link_libraries(${scene_target} PRIVATE ember_miniaudio nlohmann_json::nlohmann_json)
    endforeach()
endif()

if(MSVC)
    target_compile_options(ember_engine PRIVATE /W4 /permissive-)
    target_compile_options(ember_editor PRIVATE /W4 /permissive-)
    target_compile_options(ember_imgui PRIVATE /W4 /permissive-)
    if(BUILD_TESTING)
        target_compile_options(ember_ui_components_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_ui_input_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_audio_engine_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_physics_world_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_scene_physics_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_physics_components_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_input_map_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_scene_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_component_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_component_registry_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_script_component_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_script_runtime_scene_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_texture_cache_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_asset_directory_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_asset_manager_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_asset_registry_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_asset_metadata_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_scene_serializer_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_asset_import_service_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_project_configuration_tests PRIVATE /W4 /permissive-)
        target_compile_options(ember_lua_script_runtime_tests PRIVATE /W4 /permissive-)
    endif()
else()
    target_compile_options(ember_engine PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ember_editor PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ember_imgui PRIVATE -Wall -Wextra -Wpedantic)
    if(BUILD_TESTING)
        target_compile_options(ember_ui_components_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_ui_input_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_audio_engine_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_physics_world_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_physics_components_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_input_map_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_scene_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_component_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_component_registry_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_script_component_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_script_runtime_scene_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_texture_cache_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_asset_directory_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_asset_manager_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_asset_registry_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_asset_metadata_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_scene_serializer_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_asset_import_service_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_project_configuration_tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(ember_lua_script_runtime_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

set_target_properties(ember_editor PROPERTIES
    OUTPUT_NAME "ember"
)
