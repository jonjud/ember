#pragma once

#include "Engine/Platform/InputActionState.hpp"
#include "Engine/Scripting/ScriptComponent.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace ember
{
class Entity;

// Opaque, engine-owned identity for one live backend script instance.
// The scripting backend must never expose VM pointers or handles here.
struct ScriptInstanceHandle
{
    std::uint64_t value = 0;
    [[nodiscard]] explicit operator bool() const noexcept { return value != 0; }
    friend bool operator==(ScriptInstanceHandle, ScriptInstanceHandle) = default;
};

struct ScriptExecutionContext
{
    Entity& entity;
    float deltaSeconds = 0.0F;
    const InputActionState& input;
};

// Language-neutral boundary between EMBER runtime and a future scripting
// backend. Implementations own source loading, VM state, script instances,
// exception translation and backend-specific resources.
class ScriptRuntime
{
public:
    virtual ~ScriptRuntime() = default;

    [[nodiscard]] virtual bool Initialize(std::string* error = nullptr) = 0;
    virtual void Shutdown() noexcept = 0;

    [[nodiscard]] virtual std::optional<ScriptInstanceHandle> CreateInstance(
        const ScriptComponent& component, Entity& owner, std::string* error = nullptr) = 0;
    [[nodiscard]] virtual bool InvokeOnCreate(
        ScriptInstanceHandle instance, Entity& owner, std::string* error = nullptr) = 0;
    [[nodiscard]] virtual bool InvokeOnUpdate(
        ScriptInstanceHandle instance, const ScriptExecutionContext& context,
        std::string* error = nullptr) = 0;
    virtual void InvokeOnDestroy(ScriptInstanceHandle instance, Entity& owner) noexcept = 0;
};
} // namespace ember
