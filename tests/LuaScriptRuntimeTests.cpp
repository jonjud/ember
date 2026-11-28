#include "Engine/Resources/AssetMetadataStore.hpp"
#include "Engine/Resources/AssetRegistry.hpp"
#include "Engine/Platform/InputMap.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Scripting/LuaScriptRuntime.hpp"
#include "Engine/Scripting/ScriptComponent.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace
{
int failures = 0;
void Check(bool condition, const char* message)
{
    if (!condition) { std::cerr << "[FAIL] " << message << '\n'; ++failures; }
    else std::cout << "[PASS] " << message << '\n';
}
}

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "ember_lua_script_runtime_tests";
    std::error_code filesystemError;
    std::filesystem::remove_all(root, filesystemError);
    std::filesystem::create_directories(root / "scripts", filesystemError);

    {
        std::ofstream script(root / "scripts" / "move.lua");
        script << R"lua(
function OnCreate()
    entity:set_position(10, 20)
    assert(string.dump == nil, "string.dump must be unavailable")
    local textChunk = load("return 42", "text-only", "b")
    assert(textChunk ~= nil and textChunk() == 42, "load must force text mode")
    local binaryChunk = load("\27Lua")
    assert(binaryChunk == nil, "load must reject binary chunks")
end

function OnUpdate(deltaSeconds)
    local x, y = entity:get_position()
    entity:set_position(x + deltaSeconds * 100, y)
    if input.was_pressed("Jump") then
        entity:set_rotation(1.5)
    end
end

function OnDestroy()
    entity:set_position(99, 88)
end
)lua";
    }
    {
        std::ofstream script(root / "scripts" / "loop.lua");
        script << "function OnUpdate(deltaSeconds)\n"
                  "  local ok = pcall(function() while true do end end)\n"
                  "  if not ok then entity:set_position(777, 777) end\n"
                  "end\n";
    }
    {
        std::ofstream script(root / "scripts" / "xloop.lua");
        script << "function OnUpdate(deltaSeconds)\n"
                  "  local ok = xpcall(function() while true do end end, function(err) return err end)\n"
                  "  if not ok then entity:set_position(888, 888) end\n"
                  "end\n";
    }
    {
        std::ofstream script(root / "scripts" / "invalid.lua");
        script << "function OnUpdate(\n";
    }

    ember::AssetMetadata moveMetadata;
    moveMetadata.assetId = "script-move";
    moveMetadata.type = ember::AssetType::Script;
    moveMetadata.scriptLanguage = "lua";
    moveMetadata.source = "scripts/move.lua";
    Check(ember::AssetMetadataStore::Save(
        root / "scripts" / "move.lua.emberasset", moveMetadata), "Move script metadata saved");

    ember::AssetMetadata loopMetadata;
    loopMetadata.assetId = "script-loop";
    loopMetadata.type = ember::AssetType::Script;
    loopMetadata.scriptLanguage = "lua";
    loopMetadata.source = "scripts/loop.lua";
    Check(ember::AssetMetadataStore::Save(
        root / "scripts" / "loop.lua.emberasset", loopMetadata), "Loop script metadata saved");

    ember::AssetMetadata xloopMetadata;
    xloopMetadata.assetId = "script-xloop";
    xloopMetadata.type = ember::AssetType::Script;
    xloopMetadata.scriptLanguage = "lua";
    xloopMetadata.source = "scripts/xloop.lua";
    Check(ember::AssetMetadataStore::Save(
        root / "scripts" / "xloop.lua.emberasset", xloopMetadata), "xpcall loop metadata saved");

    ember::AssetMetadata invalidMetadata;
    invalidMetadata.assetId = "script-invalid";
    invalidMetadata.type = ember::AssetType::Script;
    invalidMetadata.scriptLanguage = "lua";
    invalidMetadata.source = "scripts/invalid.lua";
    Check(ember::AssetMetadataStore::Save(
        root / "scripts" / "invalid.lua.emberasset", invalidMetadata), "Invalid script metadata saved");

    ember::AssetMetadata pythonMetadata;
    pythonMetadata.assetId = "script-python";
    pythonMetadata.type = ember::AssetType::Script;
    pythonMetadata.source = "scripts/other.py";
    pythonMetadata.scriptLanguage = "python";
    Check(ember::AssetMetadataStore::Save(
        root / "scripts" / "other.py.emberasset", pythonMetadata), "Python metadata fixture saved");

    std::ofstream(root / "scripts" / "other.py") << "print('not lua')";

    ember::AssetRegistry assets(root);
    Check(assets.Scan(), "Script asset registry scans successfully");

    ember::Scene scene("Lua Runtime Tests");
    auto& actor = scene.CreateEntity("Actor");
    auto& script = actor.AddComponent<ember::ScriptComponent>();
    script.scriptAssetId = "script-move";

    auto& loopActor = scene.CreateEntity("Loop");
    auto& loopScript = loopActor.AddComponent<ember::ScriptComponent>();
    loopScript.scriptAssetId = "script-loop";

    auto& xloopActor = scene.CreateEntity("XLoop");
    auto& xloopScript = xloopActor.AddComponent<ember::ScriptComponent>();
    xloopScript.scriptAssetId = "script-xloop";

    auto& pythonActor = scene.CreateEntity("Python");
    auto& pythonScript = pythonActor.AddComponent<ember::ScriptComponent>();
    pythonScript.scriptAssetId = "script-python";

    auto& invalidActor = scene.CreateEntity("Invalid");
    auto& invalidScript = invalidActor.AddComponent<ember::ScriptComponent>();
    invalidScript.scriptAssetId = "script-invalid";

    ember::LuaScriptRuntime runtime(assets);
    scene.Start(nullptr, &assets, &runtime);
    Check(scene.GetScriptInstanceCount() == 3,
        "Lua creates independent instances and rejects invalid source and mismatched languages");
    Check(pythonActor.GetTransform().position.x == 0.0F,
        "Lua backend refuses assets tagged for a different scripting language");
    Check(actor.GetTransform().position.x == 10.0F &&
          actor.GetTransform().position.y == 20.0F,
        "OnCreate can read and modify its owning Entity Transform");

    ember::InputMap input;
    (void)input.BindAction("Jump", SDL_SCANCODE_SPACE);
    input.Update({SDL_SCANCODE_SPACE});
    scene.Update(0.5F, input.GetActionState());
    Check(actor.GetTransform().position.x == 60.0F &&
          actor.GetTransform().rotation == 1.5F,
        "OnUpdate receives delta time and named input action edges");
    Check(!scene.GetScriptDiagnostics().empty(),
        "Instruction budget violations escape nested pcall and stop the callback");
    Check(loopActor.GetTransform().position.x == 0.0F &&
          loopActor.GetTransform().position.y == 0.0F,
        "pcall cannot swallow an instruction-budget violation and continue execution");
    Check(xloopActor.GetTransform().position.x == 0.0F &&
          xloopActor.GetTransform().position.y == 0.0F,
        "xpcall cannot swallow an instruction-budget violation and continue execution");

    input.Update({});
    scene.Update(0.25F, input.GetActionState());
    Check(actor.GetTransform().position.x == 85.0F,
        "A faulted Lua instance is isolated while healthy scripts keep updating");

    scene.Stop();
    Check(actor.GetTransform().position.x == 99.0F &&
          actor.GetTransform().position.y == 88.0F,
        "OnDestroy runs before the owning Scene releases script instances");

    std::filesystem::remove_all(root, filesystemError);
    if (failures != 0) return 1;
    std::cout << "All Lua ScriptRuntime tests passed.\n";
    return 0;
}
