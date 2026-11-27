#pragma once

#include "Engine/Scene/Component.hpp"
#include "Engine/Math/Vector2.hpp"
#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <functional>
#include <limits>
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
    std::uint32_t schemaVersion = 1;
};

enum class ComponentPropertyType
{
    Boolean,
    Integer,
    Float,
    String,
    Vector2,
    Enumeration,
    EntityReference
};

struct ComponentPropertyInfo
{
    std::string id;
    std::string displayName;
    ComponentPropertyType type = ComponentPropertyType::String;
    std::vector<std::pair<int, std::string>> enumValues;
    std::function<nlohmann::json(const Component&)> get;
    std::function<bool(Component&, const nlohmann::json&)> set;
};

// Explicit registry for component types that are safe to create from tools such
// as the Editor. Registration is deterministic and rejects duplicate IDs/types.
// Property descriptors are data-only callbacks; the engine does not depend on ImGui.
class ComponentRegistry
{
public:
    using Json = nlohmann::json;
    using SerializeFunction = std::function<Json(const Component&)>;
    using DeserializeFunction = std::function<bool(Component&, const Json&)>;
    using MigrationFunction = std::function<bool(Json&)>;

    template <typename T>
    bool Register(std::string id, std::string displayName, SerializeFunction serialize = {},
        DeserializeFunction deserialize = {}, std::uint32_t schemaVersion = 1)
    {
        static_assert(std::is_base_of_v<Component, T>,
            "Registered types must derive from ember::Component");
        static_assert(std::is_default_constructible_v<T>,
            "Registered components must be default constructible");

        if (id.empty() || displayName.empty() || schemaVersion == 0)
            return false;

        const std::type_index type{typeid(T)};
        if (m_types.contains(id) || m_idsByType.contains(type))
            return false;

        Entry entry;
        entry.info = ComponentTypeInfo{std::move(id), std::move(displayName), type, schemaVersion};
        entry.factory = [] { return std::make_unique<T>(); };
        entry.serialize = std::move(serialize);
        entry.deserialize = std::move(deserialize);
        const std::string registeredId = entry.info.id;
        m_idsByType.emplace(type, registeredId);
        m_types.emplace(registeredId, std::move(entry));
        return true;
    }

    bool RegisterMigration(const std::string& componentId, std::uint32_t fromVersion,
        MigrationFunction migration)
    {
        const auto entryIt = m_types.find(componentId);
        if (entryIt == m_types.end() || fromVersion == 0 ||
            fromVersion >= entryIt->second.info.schemaVersion || !migration)
            return false;
        return entryIt->second.migrations.emplace(fromVersion, std::move(migration)).second;
    }

    template <typename T, typename Value>
    bool RegisterProperty(const std::string& componentId, std::string propertyId,
        std::string displayName, Value T::* member)
    {
        static_assert(std::is_base_of_v<Component, T>,
            "Property owner must derive from ember::Component");
        static_assert(std::is_same_v<Value, bool> || std::is_same_v<Value, int> ||
            std::is_same_v<Value, float> || std::is_same_v<Value, std::string>,
            "Component properties support bool, int, float, and std::string");

        const auto entryIt = m_types.find(componentId);
        if (entryIt == m_types.end() || entryIt->second.info.cppType != std::type_index(typeid(T)) ||
            propertyId.empty() || displayName.empty() || member == nullptr)
            return false;

        auto& properties = entryIt->second.properties;
        for (const auto& property : properties)
            if (property.id == propertyId) return false;

        ComponentPropertyInfo property;
        property.id = std::move(propertyId);
        property.displayName = std::move(displayName);
        if constexpr (std::is_same_v<Value, bool>)
            property.type = ComponentPropertyType::Boolean;
        else if constexpr (std::is_same_v<Value, int>)
            property.type = ComponentPropertyType::Integer;
        else if constexpr (std::is_same_v<Value, float>)
            property.type = ComponentPropertyType::Float;
        else
            property.type = ComponentPropertyType::String;

        property.get = [member](const Component& component) -> Json
        {
            return Json(static_cast<const T&>(component).*member);
        };
        property.set = [member](Component& component, const Json& value) -> bool
        {
            if constexpr (std::is_same_v<Value, bool>)
            {
                if (!value.is_boolean()) return false;
            }
            else if constexpr (std::is_same_v<Value, int>)
            {
                if (!value.is_number_integer()) return false;
                try
                {
                    const auto number = value.get<long long>();
                    if (number < std::numeric_limits<int>::min() ||
                        number > std::numeric_limits<int>::max()) return false;
                    static_cast<T&>(component).*member = static_cast<int>(number);
                    return true;
                }
                catch (...) { return false; }
            }
            else if constexpr (std::is_same_v<Value, float>)
            {
                if (!value.is_number()) return false;
                try
                {
                    const double number = value.get<double>();
                    if (number < -static_cast<double>(std::numeric_limits<float>::max()) ||
                        number > static_cast<double>(std::numeric_limits<float>::max())) return false;
                    static_cast<T&>(component).*member = static_cast<float>(number);
                    return true;
                }
                catch (...) { return false; }
            }
            else
            {
                if (!value.is_string()) return false;
                try
                {
                    std::string text = value.get<std::string>();
                    if (text.size() > 4096) return false;
                    static_cast<T&>(component).*member = std::move(text);
                    return true;
                }
                catch (...) { return false; }
            }

            static_cast<T&>(component).*member = value.get<Value>();
            return true;
        };

        properties.push_back(std::move(property));
        return true;
    }



    template <typename T>
    bool RegisterEntityReferenceProperty(const std::string& componentId, std::string propertyId,
        std::string displayName, std::uint64_t T::* member)
    {
        static_assert(std::is_base_of_v<Component, T>,
            "Property owner must derive from ember::Component");
        const auto entryIt = m_types.find(componentId);
        if (entryIt == m_types.end() ||
            entryIt->second.info.cppType != std::type_index(typeid(T)) ||
            propertyId.empty() || displayName.empty() || member == nullptr)
            return false;
        auto& properties = entryIt->second.properties;
        for (const auto& property : properties)
            if (property.id == propertyId) return false;

        ComponentPropertyInfo property;
        property.id = std::move(propertyId);
        property.displayName = std::move(displayName);
        property.type = ComponentPropertyType::EntityReference;
        property.get = [member](const Component& component) -> Json {
            return Json(static_cast<const T&>(component).*member);
        };
        property.set = [member](Component& component, const Json& value) -> bool {
            if (!value.is_number_unsigned() && !value.is_number_integer()) return false;
            try {
                if (value.is_number_integer() && value.get<std::int64_t>() < 0) return false;
                static_cast<T&>(component).*member = value.get<std::uint64_t>();
                return true;
            } catch (...) { return false; }
        };
        properties.push_back(std::move(property));
        return true;
    }

    template <typename T>
    bool RegisterVector2Property(const std::string& componentId, std::string propertyId,
        std::string displayName, Vector2 T::* member)
    {
        static_assert(std::is_base_of_v<Component, T>,
            "Property owner must derive from ember::Component");
        const auto entryIt = m_types.find(componentId);
        if (entryIt == m_types.end() || entryIt->second.info.cppType != std::type_index(typeid(T)) ||
            propertyId.empty() || displayName.empty() || member == nullptr)
            return false;
        auto& properties = entryIt->second.properties;
        for (const auto& property : properties)
            if (property.id == propertyId) return false;

        ComponentPropertyInfo property;
        property.id = std::move(propertyId);
        property.displayName = std::move(displayName);
        property.type = ComponentPropertyType::Vector2;
        property.get = [member](const Component& component) -> Json {
            const Vector2 value = static_cast<const T&>(component).*member;
            return Json::array({value.x, value.y});
        };
        property.set = [member](Component& component, const Json& value) -> bool {
            if (!value.is_array() || value.size() != 2 ||
                !value[0].is_number() || !value[1].is_number()) return false;
            try {
                const double x = value[0].get<double>();
                const double y = value[1].get<double>();
                if (!std::isfinite(x) || !std::isfinite(y) ||
                    std::abs(x) > std::numeric_limits<float>::max() ||
                    std::abs(y) > std::numeric_limits<float>::max()) return false;
                static_cast<T&>(component).*member = Vector2{
                    static_cast<float>(x), static_cast<float>(y)};
                return true;
            } catch (...) { return false; }
        };
        properties.push_back(std::move(property));
        return true;
    }

    template <typename T, typename Enum>
    bool RegisterEnumProperty(const std::string& componentId, std::string propertyId,
        std::string displayName, Enum T::* member,
        std::vector<std::pair<Enum, std::string>> options)
    {
        static_assert(std::is_base_of_v<Component, T>,
            "Property owner must derive from ember::Component");
        static_assert(std::is_enum_v<Enum>, "Enumeration properties require an enum member");
        const auto entryIt = m_types.find(componentId);
        if (entryIt == m_types.end() || entryIt->second.info.cppType != std::type_index(typeid(T)) ||
            propertyId.empty() || displayName.empty() || member == nullptr || options.empty())
            return false;
        auto& properties = entryIt->second.properties;
        for (const auto& property : properties)
            if (property.id == propertyId) return false;

        ComponentPropertyInfo property;
        property.id = std::move(propertyId);
        property.displayName = std::move(displayName);
        property.type = ComponentPropertyType::Enumeration;
        for (const auto& option : options) {
            const int value = static_cast<int>(option.first);
            if (option.second.empty()) return false;
            for (const auto& existing : property.enumValues)
                if (existing.first == value) return false;
            property.enumValues.emplace_back(value, option.second);
        }
        property.get = [member](const Component& component) -> Json {
            return static_cast<int>(static_cast<const T&>(component).*member);
        };
        const auto allowedValues = property.enumValues;
        property.set = [member, allowedValues](Component& component, const Json& value) -> bool {
            if (!value.is_number_integer()) return false;
            try {
                const int selected = value.get<int>();
                for (const auto& option : allowedValues) {
                    if (option.first == selected) {
                        static_cast<T&>(component).*member = static_cast<Enum>(selected);
                        return true;
                    }
                }
            } catch (...) { return false; }
            return false;
        };
        properties.push_back(std::move(property));
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

    [[nodiscard]] const std::vector<ComponentPropertyInfo>* FindProperties(const Component& component) const
    {
        const auto* info = Find(component);
        if (!info) return nullptr;
        const auto it = m_types.find(info->id);
        return it == m_types.end() ? nullptr : &it->second.properties;
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
        if (it == m_types.end()) return nullptr;
        return Create(id, it->second.info.schemaVersion, data);
    }

    [[nodiscard]] std::unique_ptr<Component> Create(
        const std::string& id, std::uint32_t savedVersion, const Json& savedData) const
    {
        const auto it = m_types.find(id);
        if (it == m_types.end() || !it->second.deserialize ||
            savedVersion == 0 || savedVersion > it->second.info.schemaVersion)
            return nullptr;

        Json migratedData = savedData;
        try
        {
            for (std::uint32_t version = savedVersion;
                 version < it->second.info.schemaVersion; ++version)
            {
                const auto migration = it->second.migrations.find(version);
                if (migration == it->second.migrations.end() ||
                    !migration->second(migratedData))
                    return nullptr;
            }

            auto component = it->second.factory();
            if (!component || !it->second.deserialize(*component, migratedData))
                return nullptr;
            return component;
        }
        catch (...)
        {
            return nullptr;
        }
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
        std::map<std::uint32_t, MigrationFunction> migrations;
        std::vector<ComponentPropertyInfo> properties;
    };

    std::map<std::string, Entry> m_types;
    std::map<std::type_index, std::string> m_idsByType;
};
} // namespace ember
