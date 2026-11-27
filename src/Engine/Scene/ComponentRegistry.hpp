#pragma once

#include "Engine/Scene/Component.hpp"
#include <nlohmann/json.hpp>

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <typeindex>
#include <type_traits>
#include <utility>
#include <vector>

namespace ember
{
struct ComponentTypeInfo
{
    std::string id;
    std::string displayName;
    std::type_index cppType{typeid(void)};
};

// Explicit registry for component types that are safe to create from tools such
// as the Editor. Registration is deterministic and rejects duplicate IDs/types.
class ComponentRegistry
{
public:
    using Json = nlohmann::json;
    using SerializeFunction = std::function<Json(const Component&)>;
    using DeserializeFunction = std::function<bool(Component&, const Json&)>;

    template <typename T>
    bool Register(std::string id, std::string displayName, SerializeFunction serialize = {}, DeserializeFunction deserialize = {})
    {
        static_assert(std::is_base_of_v<Component, T>,
            "Registered types must derive from ember::Component");
        static_assert(std::is_default_constructible_v<T>,
            "Registered components must be default constructible");

        if (id.empty() || displayName.empty())
            return false;

        const std::type_index type{typeid(T)};
        if (m_types.contains(id) || m_idsByType.contains(type))
            return false;

        Entry entry;
        entry.info = ComponentTypeInfo{std::move(id), std::move(displayName), type};
        entry.factory = [] { return std::make_unique<T>(); };
        entry.serialize = std::move(serialize);
        entry.deserialize = std::move(deserialize);
        const std::string registeredId = entry.info.id;
        m_idsByType.emplace(type, registeredId);
        m_types.emplace(registeredId, std::move(entry));
        return true;
    }

    [[nodiscard]] std::unique_ptr<Component> Create(const std::string& id) const
    {
        const auto it = m_types.find(id);
        return it == m_types.end() ? nullptr : it->second.factory();
    }

    [[nodiscard]] const ComponentTypeInfo* Find(const std::string& id) const
    {
        const auto it = m_types.find(id);
        return it == m_types.end() ? nullptr : &it->second.info;
    }

    [[nodiscard]] const ComponentTypeInfo* Find(const Component& component) const
    {
        const auto it = m_idsByType.find(std::type_index(typeid(component)));
        return it == m_idsByType.end() ? nullptr : Find(it->second);
    }

    [[nodiscard]] std::vector<ComponentTypeInfo> GetRegisteredTypes() const
    {
        std::vector<ComponentTypeInfo> result;
        result.reserve(m_types.size());
        for (const auto& [id, entry] : m_types)
        {
            (void)id;
            result.push_back(entry.info);
        }
        return result;
    }

    [[nodiscard]] bool Serialize(const Component& component, Json& data) const
    {
        const auto* info = Find(component);
        if (!info) return false;
        const auto it = m_types.find(info->id);
        if (it == m_types.end() || !it->second.serialize || !it->second.deserialize) return false;
        data = it->second.serialize(component);
        return true;
    }

    [[nodiscard]] std::unique_ptr<Component> Create(const std::string& id, const Json& data) const
    {
        const auto it = m_types.find(id);
        if (it == m_types.end() || !it->second.deserialize) return nullptr;
        auto component = it->second.factory();
        if (!component || !it->second.deserialize(*component, data)) return nullptr;
        return component;
    }

    [[nodiscard]] std::size_t Size() const { return m_types.size(); }
    [[nodiscard]] bool IsEmpty() const { return m_types.empty(); }

private:
    struct Entry
    {
        ComponentTypeInfo info;
        std::function<std::unique_ptr<Component>()> factory;
        SerializeFunction serialize;
        DeserializeFunction deserialize;
    };

    std::map<std::string, Entry> m_types;
    std::map<std::type_index, std::string> m_idsByType;
};
} // namespace ember
