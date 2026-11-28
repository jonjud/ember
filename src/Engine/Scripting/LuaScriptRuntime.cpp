#include "Engine/Scripting/LuaScriptRuntime.hpp"

#include "Engine/Scene/Entity.hpp"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <new>
#include <string>
#include <system_error>
#include <utility>

namespace ember
{
namespace
{
constexpr std::size_t kMaximumScriptBytes = 1024U * 1024U;
constexpr std::size_t kMaximumInstanceMemory = 16U * 1024U * 1024U;
constexpr std::size_t kInstructionBudget = 1'000'000U;
constexpr int kInstructionHookInterval = 1'000;

bool Fail(std::string* error, const std::string& message)
{
    if (error != nullptr) *error = message;
    return false;
}

std::string LuaError(lua_State* state)
{
    const char* message = lua_tostring(state, -1);
    return message != nullptr ? std::string(message) : std::string("Unknown Lua error.");
}

void OpenSafeLibrary(lua_State* state, const char* name, lua_CFunction openFunction)
{
    luaL_requiref(state, name, openFunction, 1);
    lua_pop(state, 1);
}
}

struct LuaScriptRuntime::Instance
{
    lua_State* state = nullptr;
    Entity* owner = nullptr;
    const InputActionState* input = nullptr;
    std::size_t memoryUsed = 0;
    std::size_t instructionsRemaining = kInstructionBudget;

    static void* Allocate(void* userData, void* pointer, std::size_t oldSize, std::size_t newSize)
    {
        auto* instance = static_cast<Instance*>(userData);
        const std::size_t trackedOldSize = pointer != nullptr
            ? std::min(oldSize, instance->memoryUsed) : 0U;
        const std::size_t retainedSize = instance->memoryUsed - trackedOldSize;

        if (newSize == 0)
        {
            std::free(pointer);
            instance->memoryUsed = retainedSize;
            return nullptr;
        }
        if (newSize > kMaximumInstanceMemory - retainedSize)
            return nullptr;

        void* result = std::realloc(pointer, newSize);
        if (result != nullptr)
            instance->memoryUsed = retainedSize + newSize;
        return result;
    }

    static Instance* FromUpvalue(lua_State* state)
    {
        return static_cast<Instance*>(lua_touserdata(state, lua_upvalueindex(1)));
    }

    static void InstructionHook(lua_State* state, lua_Debug*)
    {
        auto* instance = *static_cast<Instance**>(lua_getextraspace(state));
        if (instance == nullptr) return;
        if (instance->instructionsRemaining <= static_cast<std::size_t>(kInstructionHookInterval))
            luaL_error(state, "Script instruction budget exceeded.");
        instance->instructionsRemaining -= static_cast<std::size_t>(kInstructionHookInterval);
    }

    static int GetName(lua_State* state)
    {
        const auto* instance = FromUpvalue(state);
        if (instance == nullptr || instance->owner == nullptr)
            return luaL_error(state, "Entity is no longer available.");
        const std::string& name = instance->owner->GetName();
        lua_pushlstring(state, name.data(), name.size());
        return 1;
    }

    static int GetPosition(lua_State* state)
    {
        const auto* instance = FromUpvalue(state);
        if (instance == nullptr || instance->owner == nullptr)
            return luaL_error(state, "Entity is no longer available.");
        const Vector2 position = instance->owner->GetTransform().position;
        lua_pushnumber(state, position.x);
        lua_pushnumber(state, position.y);
        return 2;
    }

    static int SetPosition(lua_State* state)
    {
        auto* instance = FromUpvalue(state);
        if (instance == nullptr || instance->owner == nullptr)
            return luaL_error(state, "Entity is no longer available.");
        const lua_Number x = luaL_checknumber(state, 2);
        const lua_Number y = luaL_checknumber(state, 3);
        if (!std::isfinite(x) || !std::isfinite(y))
            return luaL_error(state, "Entity position must contain finite numbers.");
        instance->owner->GetTransform().position = {
            static_cast<float>(x), static_cast<float>(y)};
        return 0;
    }

    static int GetRotation(lua_State* state)
    {
        const auto* instance = FromUpvalue(state);
        if (instance == nullptr || instance->owner == nullptr)
            return luaL_error(state, "Entity is no longer available.");
        lua_pushnumber(state, instance->owner->GetTransform().rotation);
        return 1;
    }

    static int SetRotation(lua_State* state)
    {
        auto* instance = FromUpvalue(state);
        if (instance == nullptr || instance->owner == nullptr)
            return luaL_error(state, "Entity is no longer available.");
        const lua_Number rotation = luaL_checknumber(state, 2);
        if (!std::isfinite(rotation))
            return luaL_error(state, "Entity rotation must be finite.");
        instance->owner->GetTransform().rotation = static_cast<float>(rotation);
        return 0;
    }

    static int GetScale(lua_State* state)
    {
        const auto* instance = FromUpvalue(state);
        if (instance == nullptr || instance->owner == nullptr)
            return luaL_error(state, "Entity is no longer available.");
        const Vector2 scale = instance->owner->GetTransform().scale;
        lua_pushnumber(state, scale.x);
        lua_pushnumber(state, scale.y);
        return 2;
    }

    static int SetScale(lua_State* state)
    {
        auto* instance = FromUpvalue(state);
        if (instance == nullptr || instance->owner == nullptr)
            return luaL_error(state, "Entity is no longer available.");
        const lua_Number x = luaL_checknumber(state, 2);
        const lua_Number y = luaL_checknumber(state, 3);
        if (!std::isfinite(x) || !std::isfinite(y))
            return luaL_error(state, "Entity scale must contain finite numbers.");
        instance->owner->GetTransform().scale = {
            static_cast<float>(x), static_cast<float>(y)};
        return 0;
    }

    static int InputIsDown(lua_State* state)
    {
        const auto* instance = FromUpvalue(state);
        const char* action = luaL_checkstring(state, 1);
        lua_pushboolean(state, instance != nullptr && instance->input != nullptr &&
            instance->input->IsDown(action));
        return 1;
    }

    static int InputWasPressed(lua_State* state)
    {
        const auto* instance = FromUpvalue(state);
        const char* action = luaL_checkstring(state, 1);
        lua_pushboolean(state, instance != nullptr && instance->input != nullptr &&
            instance->input->WasPressed(action));
        return 1;
    }

    static int InputWasReleased(lua_State* state)
    {
        const auto* instance = FromUpvalue(state);
        const char* action = luaL_checkstring(state, 1);
        lua_pushboolean(state, instance != nullptr && instance->input != nullptr &&
            instance->input->WasReleased(action));
        return 1;
    }

    static void SetClosure(lua_State* state, const char* name, lua_CFunction function,
        Instance* instance)
    {
        lua_pushlightuserdata(state, instance);
        lua_pushcclosure(state, function, 1);
        lua_setfield(state, -2, name);
    }

    static void RegisterHostApi(Instance& instance)
    {
        lua_State* state = instance.state;

        lua_newtable(state);
        lua_pushinteger(state, static_cast<lua_Integer>(instance.owner->GetId()));
        lua_setfield(state, -2, "id");
        const std::string& name = instance.owner->GetName();
        lua_pushlstring(state, name.data(), name.size());
        lua_setfield(state, -2, "name");
        SetClosure(state, "get_name", GetName, &instance);
        SetClosure(state, "get_position", GetPosition, &instance);
        SetClosure(state, "set_position", SetPosition, &instance);
        SetClosure(state, "get_rotation", GetRotation, &instance);
        SetClosure(state, "set_rotation", SetRotation, &instance);
        SetClosure(state, "get_scale", GetScale, &instance);
        SetClosure(state, "set_scale", SetScale, &instance);
        lua_setglobal(state, "entity");

        lua_newtable(state);
        SetClosure(state, "is_down", InputIsDown, &instance);
        SetClosure(state, "was_pressed", InputWasPressed, &instance);
        SetClosure(state, "was_released", InputWasReleased, &instance);
        lua_setglobal(state, "input");
    }
};

LuaScriptRuntime::LuaScriptRuntime(const AssetRegistry& assets)
    : m_assets(assets)
{
}

LuaScriptRuntime::~LuaScriptRuntime()
{
    Shutdown();
}

bool LuaScriptRuntime::Initialize(std::string* error)
{
    if (m_initialized)
    {
        if (error != nullptr) error->clear();
        return true;
    }
    m_instances.clear();
    m_nextHandle = 1;
    m_initialized = true;
    if (error != nullptr) error->clear();
    return true;
}

void LuaScriptRuntime::Shutdown() noexcept
{
    for (auto& [id, instance] : m_instances)
    {
        (void)id;
        if (instance && instance->state != nullptr)
        {
            lua_close(instance->state);
            instance->state = nullptr;
        }
    }
    m_instances.clear();
    m_initialized = false;
}

std::optional<ScriptInstanceHandle> LuaScriptRuntime::CreateInstance(
    const ScriptComponent& component, Entity& owner, std::string* error)
{
    if (!m_initialized)
    {
        Fail(error, "LuaScriptRuntime has not been initialized.");
        return std::nullopt;
    }
    if (!component.IsConfigurationValid() || component.scriptAssetId.empty())
    {
        Fail(error, "ScriptComponent has an invalid or empty Script asset ID.");
        return std::nullopt;
    }

    const AssetMetadata* metadata = m_assets.Find(component.scriptAssetId);
    if (metadata == nullptr || metadata->type != AssetType::Script)
    {
        Fail(error, "Script asset ID was not found or does not refer to a Script asset.");
        return std::nullopt;
    }
    const auto sourcePath = m_assets.GetDirectory().Resolve(metadata->source);
    if (!sourcePath)
    {
        Fail(error, "Script asset source path is not safe.");
        return std::nullopt;
    }

    std::error_code filesystemError;
    const auto sourceSize = std::filesystem::file_size(*sourcePath, filesystemError);
    if (filesystemError || sourceSize > kMaximumScriptBytes)
    {
        Fail(error, "Script source is missing, unreadable or exceeds the 1 MiB source limit.");
        return std::nullopt;
    }

    std::ifstream source(*sourcePath, std::ios::binary);
    if (!source)
    {
        Fail(error, "Script source could not be opened.");
        return std::nullopt;
    }
    std::string sourceText(static_cast<std::size_t>(sourceSize), '\0');
    if (!sourceText.empty())
        source.read(sourceText.data(), static_cast<std::streamsize>(sourceText.size()));
    if (source.gcount() != static_cast<std::streamsize>(sourceText.size()))
    {
        Fail(error, "Script source could not be read completely.");
        return std::nullopt;
    }
    if (sourceText.find('\0') != std::string::npos)
    {
        Fail(error, "Script source contains a NUL byte and is not accepted as text.");
        return std::nullopt;
    }

    auto instance = std::make_unique<Instance>();
    instance->owner = &owner;
    instance->state = lua_newstate(&Instance::Allocate, instance.get(), 0U);
    if (instance->state == nullptr)
    {
        Fail(error, "Lua could not create a VM within the per-script memory budget.");
        return std::nullopt;
    }
    *static_cast<Instance**>(lua_getextraspace(instance->state)) = instance.get();

    OpenSafeLibrary(instance->state, LUA_GNAME, luaopen_base);
    OpenSafeLibrary(instance->state, LUA_TABLIBNAME, luaopen_table);
    OpenSafeLibrary(instance->state, LUA_STRLIBNAME, luaopen_string);
    OpenSafeLibrary(instance->state, LUA_MATHLIBNAME, luaopen_math);
    OpenSafeLibrary(instance->state, LUA_UTF8LIBNAME, luaopen_utf8);

    // The base library includes file-loading helpers; scripts may not access the host filesystem.
    lua_pushnil(instance->state);
    lua_setglobal(instance->state, "dofile");
    lua_pushnil(instance->state);
    lua_setglobal(instance->state, "loadfile");

    Instance::RegisterHostApi(*instance);
    instance->instructionsRemaining = kInstructionBudget;
    lua_sethook(instance->state, &Instance::InstructionHook, LUA_MASKCOUNT, kInstructionHookInterval);
    if (luaL_loadbufferx(instance->state, sourceText.data(), sourceText.size(),
            metadata->source.generic_string().c_str(), "t") != LUA_OK ||
        lua_pcall(instance->state, 0, 0, 0) != LUA_OK)
    {
        const std::string message = LuaError(instance->state);
        lua_close(instance->state);
        instance->state = nullptr;
        Fail(error, "Lua script load failed (" + metadata->source.generic_string() + "): " + message);
        return std::nullopt;
    }
    lua_sethook(instance->state, nullptr, 0, 0);

    if (m_nextHandle == 0)
    {
        lua_close(instance->state);
        instance->state = nullptr;
        Fail(error, "Lua script instance handle space is exhausted.");
        return std::nullopt;
    }
    const ScriptInstanceHandle handle{m_nextHandle++};
    m_instances.emplace(handle.value, std::move(instance));
    if (error != nullptr) error->clear();
    return handle;
}

LuaScriptRuntime::Instance* LuaScriptRuntime::FindInstance(ScriptInstanceHandle handle) noexcept
{
    const auto it = m_instances.find(handle.value);
    return it != m_instances.end() ? it->second.get() : nullptr;
}

bool LuaScriptRuntime::CallCallback(
    Instance& instance, const char* callback, float deltaSeconds,
    const InputActionState* input, std::string* error)
{
    if (instance.state == nullptr) return Fail(error, "Lua VM is not available.");
    lua_State* state = instance.state;
    instance.input = input;
    instance.instructionsRemaining = kInstructionBudget;
    lua_sethook(state, &Instance::InstructionHook, LUA_MASKCOUNT, kInstructionHookInterval);

    lua_getglobal(state, callback);
    if (lua_isnil(state, -1))
    {
        lua_pop(state, 1);
        instance.input = nullptr;
        lua_sethook(state, nullptr, 0, 0);
        if (error != nullptr) error->clear();
        return true;
    }
    if (!lua_isfunction(state, -1))
    {
        lua_pop(state, 1);
        instance.input = nullptr;
        lua_sethook(state, nullptr, 0, 0);
        return Fail(error, std::string("Lua callback '") + callback + "' must be a function.");
    }

    int argumentCount = 0;
    if (std::string(callback) == "OnUpdate")
    {
        lua_pushnumber(state, deltaSeconds);
        argumentCount = 1;
    }
    const int result = lua_pcall(state, argumentCount, 0, 0);
    instance.input = nullptr;
    lua_sethook(state, nullptr, 0, 0);
    if (result != LUA_OK)
    {
        const std::string message = LuaError(state);
        lua_pop(state, 1);
        return Fail(error, std::string("Lua ") + callback + " failed: " + message);
    }
    if (error != nullptr) error->clear();
    return true;
}

bool LuaScriptRuntime::InvokeOnCreate(
    ScriptInstanceHandle handle, Entity& owner, std::string* error)
{
    Instance* instance = FindInstance(handle);
    if (instance == nullptr || instance->owner != &owner)
        return Fail(error, "Lua script instance handle does not belong to this Entity.");
    return CallCallback(*instance, "OnCreate", 0.0F, nullptr, error);
}

bool LuaScriptRuntime::InvokeOnUpdate(
    ScriptInstanceHandle handle, const ScriptExecutionContext& context, std::string* error)
{
    Instance* instance = FindInstance(handle);
    if (instance == nullptr || instance->owner != &context.entity)
        return Fail(error, "Lua script instance handle does not belong to this Entity.");
    return CallCallback(*instance, "OnUpdate", context.deltaSeconds, &context.input, error);
}

void LuaScriptRuntime::InvokeOnDestroy(ScriptInstanceHandle handle, Entity& owner) noexcept
{
    Instance* instance = FindInstance(handle);
    if (instance == nullptr || instance->owner != &owner) return;
    try
    {
        (void)CallCallback(*instance, "OnDestroy", 0.0F, nullptr, nullptr);
    }
    catch (...)
    {
        // Destruction is best-effort and must not escape Scene::Stop.
    }
}
} // namespace ember
