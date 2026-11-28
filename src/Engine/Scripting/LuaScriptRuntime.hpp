#pragma once

#include "Engine/Resources/AssetRegistry.hpp"
#include "Engine/Scripting/ScriptRuntime.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <memory>
#include <string>
#include <unordered_map>

namespace ember
{
// First optional gameplay backend. Lua types stay private to this implementation.
class LuaScriptRuntime final : public ScriptRuntime
{
public:
    explicit LuaScriptRuntime(const AssetRegistry& assets);
    ~LuaScriptRuntime() override;

    LuaScriptRuntime(const LuaScriptRuntime&) = delete;
    LuaScriptRuntime& operator=(const LuaScriptRuntime&) = delete;

    [[nodiscard]] bool Initialize(std::string* error = nullptr) override;
    [[nodiscard]] static bool ValidateSource(std::string_view source,
        std::string_view chunkName, std::string* error = nullptr,
        std::size_t* errorLine = nullptr, std::size_t* errorColumn = nullptr);
    void Shutdown() noexcept override;

    [[nodiscard]] std::optional<ScriptInstanceHandle> CreateInstance(
        const ScriptComponent& component, Entity& owner, std::string* error = nullptr) override;
    [[nodiscard]] bool InvokeOnCreate(
        ScriptInstanceHandle instance, Entity& owner, std::string* error = nullptr) override;
    [[nodiscard]] bool InvokeOnUpdate(
        ScriptInstanceHandle instance, const ScriptExecutionContext& context,
        std::string* error = nullptr) override;
    void InvokeOnDestroy(ScriptInstanceHandle instance, Entity& owner) noexcept override;

private:
    struct Instance;
    [[nodiscard]] Instance* FindInstance(ScriptInstanceHandle handle) noexcept;
    [[nodiscard]] bool CallCallback(
        Instance& instance, const char* callback, float deltaSeconds,
        const InputActionState* input, std::string* error);

    const AssetRegistry& m_assets;
    std::unordered_map<std::uint64_t, std::unique_ptr<Instance>> m_instances;
    std::uint64_t m_nextHandle = 1;
    bool m_initialized = false;
};
} // namespace ember
