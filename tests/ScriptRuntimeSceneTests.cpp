#include "Engine/Scene/Scene.hpp"
#include "Engine/Scripting/ScriptComponent.hpp"
#include "Engine/Platform/InputMap.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace
{
int failures = 0;
void Check(bool ok, const char* message)
{
    if (!ok) { std::cerr << "[FAIL] " << message << '\n'; ++failures; }
    else std::cout << "[PASS] " << message << '\n';
}

class MockScriptRuntime final : public ember::ScriptRuntime
{
public:
    int initialize = 0, shutdown = 0, creates = 0, updates = 0, destroys = 0;
    bool failCreate = false, failOnCreate = false, failOnUpdate = false;
    float lastDelta = -1.0F;
    bool sawJump = false;
    bool Initialize(std::string* error) override
    {
        ++initialize; if (error) error->clear(); return true;
    }
    void Shutdown() noexcept override { ++shutdown; }
    std::optional<ember::ScriptInstanceHandle> CreateInstance(
        const ember::ScriptComponent&, ember::Entity&, std::string* error) override
    {
        ++creates; if (error) *error = failCreate ? "mock create error" : "";
        if (failCreate) return std::nullopt;
        return ember::ScriptInstanceHandle{static_cast<std::uint64_t>(creates)};
    }
    bool InvokeOnCreate(ember::ScriptInstanceHandle, ember::Entity&, std::string* error) override
    {
        if (error) *error = failOnCreate ? "mock OnCreate error" : "";
        return !failOnCreate;
    }
    bool InvokeOnUpdate(ember::ScriptInstanceHandle, const ember::ScriptExecutionContext& context,
        std::string* error) override
    {
        ++updates; lastDelta = context.deltaSeconds; sawJump = context.input.IsDown("Jump");
        if (error) *error = failOnUpdate ? "mock OnUpdate error" : "";
        return !failOnUpdate;
    }
    void InvokeOnDestroy(ember::ScriptInstanceHandle, ember::Entity&) noexcept override { ++destroys; }
};
}

int main()
{
    ember::Scene scene("Script Lifecycle");
    auto& actor = scene.CreateEntity("Actor");
    auto& script = actor.AddComponent<ember::ScriptComponent>();
    script.scriptAssetId = "scripts/player";
    auto& disabledEntity = scene.CreateEntity("Disabled");
    auto& disabledScript = disabledEntity.AddComponent<ember::ScriptComponent>();
    disabledScript.scriptAssetId = "scripts/disabled";
    disabledScript.enabled = false;
    auto& unassignedEntity = scene.CreateEntity("Unassigned");
    unassignedEntity.AddComponent<ember::ScriptComponent>();
    auto& inactive = scene.CreateEntity("Inactive");
    inactive.SetActive(false);
    auto& inactiveScript = inactive.AddComponent<ember::ScriptComponent>();
    inactiveScript.scriptAssetId = "scripts/inactive";

    MockScriptRuntime runtime;
    scene.Start(nullptr, nullptr, &runtime);
    Check(runtime.initialize == 1, "Scene initializes the supplied script runtime once");
    Check(scene.GetScriptInstanceCount() == 1 && runtime.creates == 1,
        "Scene creates instances only for active enabled scripts with assigned assets");
    Check(scene.GetScriptDiagnostics().empty(), "Unassigned and disabled scripts do not produce errors");

    ember::InputMap inputMap;
    (void)inputMap.BindAction("Jump", SDL_SCANCODE_SPACE);
    inputMap.Update({SDL_SCANCODE_SPACE});
    scene.Update(0.125F, inputMap.GetActionState());
    Check(runtime.updates == 1 && runtime.lastDelta == 0.125F && runtime.sawJump,
        "Scene forwards delta time and InputActionState to script OnUpdate");

    runtime.failOnUpdate = true;
    scene.Update(0.25F);
    scene.Update(0.25F);
    Check(runtime.updates == 2 && !scene.GetScriptDiagnostics().empty(),
        "A failed OnUpdate is diagnosed and faulted instances are not called again");

    scene.Stop();
    Check(runtime.destroys == 1 && runtime.shutdown == 1,
        "Scene destroys script instances before shutting down the runtime");
    scene.Stop();
    Check(runtime.destroys == 1 && runtime.shutdown == 1,
        "Repeated Stop does not destroy or shut down twice");

    MockScriptRuntime createFailure;
    createFailure.failOnCreate = true;
    scene.Start(nullptr, nullptr, &createFailure);
    Check(scene.GetScriptInstanceCount() == 0 && createFailure.destroys == 1,
        "Failed OnCreate releases the newly created backend instance");
    Check(!scene.GetScriptDiagnostics().empty(), "Failed OnCreate is exposed as a diagnostic");
    scene.Stop();

    if (failures) return 1;
    std::cout << "All script runtime scene tests passed.\n";
    return 0;
}
