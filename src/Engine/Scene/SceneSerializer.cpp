#include "Engine/Scene/SceneSerializer.hpp"
#include "Engine/Physics/PhysicsComponents2D.hpp"
#include "Engine/Core/FileSystem.hpp"

#include <nlohmann/json.hpp>

#include <cmath>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <stdexcept>
#include <utility>

namespace ember
{
namespace
{
using Json = nlohmann::json;
constexpr std::uint32_t kSceneSchemaVersion = 1;

bool Fail(std::string* error, const std::string& message)
{
    if (error != nullptr)
    {
        *error = message;
    }
    return false;
}

bool ReadFloat(const Json& value, float& result)
{
    if (!value.is_number())
    {
        return false;
    }

    const double number = value.get<double>();
    if (!std::isfinite(number) ||
        number > static_cast<double>(std::numeric_limits<float>::max()) ||
        number < -static_cast<double>(std::numeric_limits<float>::max()))
    {
        return false;
    }

    result = static_cast<float>(number);
    return true;
}

bool ReadVector2(const Json& value, Vector2& result)
{
    return value.is_array() && value.size() == 2 &&
        ReadFloat(value[0], result.x) && ReadFloat(value[1], result.y);
}

bool ReadColor(const Json& value, std::uint8_t& result)
{
    if (value.is_number_unsigned())
    {
        const auto number = value.get<std::uint64_t>();
        if (number > 255)
        {
            return false;
        }
        result = static_cast<std::uint8_t>(number);
        return true;
    }

    if (!value.is_number_integer())
    {
        return false;
    }

    const auto number = value.get<std::int64_t>();
    if (number < 0 || number > 255)
    {
        return false;
    }

    result = static_cast<std::uint8_t>(number);
    return true;
}

bool ReadEntityId(const Json& value, EntityId& result)
{
    if (value.is_number_unsigned())
    {
        result = value.get<EntityId>();
        return result != 0;
    }

    if (!value.is_number_integer())
    {
        return false;
    }

    const auto signedId = value.get<std::int64_t>();
    if (signedId <= 0)
    {
        return false;
    }

    result = static_cast<EntityId>(signedId);
    return true;
}

bool MigrateLegacyColliders(Entity& entity, std::string* error)
{
    auto* box = entity.GetComponent<BoxCollider2D>();
    auto* circle = entity.GetComponent<CircleCollider2D>();
    auto* setExisting = entity.GetComponent<ColliderSet2D>();
    if (box && circle)
        return Fail(error, "Entity has both legacy Box and Circle colliders; resolve the conflict before migration.");
    if ((box || circle) && setExisting)
        return Fail(error, "Entity has both ColliderSet2D and a legacy collider; resolve the conflict before migration.");
    if (!box && !circle) return true;

    auto set = std::make_unique<ColliderSet2D>();
    if (box) {
        auto& c = set->AddBoxCollider();
        c.halfExtents = box->halfExtents; c.offset = box->offset; c.rotation = box->rotation;
        c.density = box->density; c.friction = box->friction;
        c.restitution = box->restitution; c.isSensor = box->isSensor;
        entity.RemoveComponent(box);
    } else {
        auto& c = set->AddCircleCollider();
        c.radius = circle->radius; c.offset = circle->offset;
        c.density = circle->density; c.friction = circle->friction;
        c.restitution = circle->restitution; c.isSensor = circle->isSensor;
        entity.RemoveComponent(circle);
    }
    entity.AddComponent(std::move(set));
    return true;
}

Json ToJson(const Entity& entity, const ComponentRegistry* componentRegistry)
{
    const Transform2D& transform = entity.GetTransform();
    const RenderComponent2D& render = entity.GetRenderComponent();
    const MovementComponent2D& movement = entity.GetMovementComponent();

    Json result{
        {"id", entity.GetId()},
        {"name", entity.GetName()},
        {"active", entity.IsActive()},
        {"movement", {{"enabled", movement.enabled}, {"speed", movement.speed}}},
        {"transform", {
            {"position", {transform.position.x, transform.position.y}},
            {"scale", {transform.scale.x, transform.scale.y}},
            {"rotation", transform.rotation}
        }},
        {"render", {
            {"size", {render.size.x, render.size.y}},
            {"tint", {render.red, render.green, render.blue, render.alpha}},
            {"visible", render.visible},
            {"texture_asset_id", render.textureAssetId}
        }}
    };

    Json components = Json::array();
    for (const auto& component : entity.GetComponents())
    {
        if (componentRegistry == nullptr)
            throw std::runtime_error("Cannot save generic components without a component registry.");
        const auto* info = componentRegistry->Find(*component);
        Json data;
        if (info == nullptr || !componentRegistry->Serialize(*component, data))
            throw std::runtime_error("Scene contains an unregistered or non-serializable component.");
        components.push_back({
            {"type", info->id},
            {"version", info->schemaVersion},
            {"data", std::move(data)}
        });
    }
    result["components"] = std::move(components);
    return result;
}
}

bool SceneSerializer::Save(
    const Scene& scene,
    const std::filesystem::path& filePath,
    std::string* error,
    const ComponentRegistry* componentRegistry)
{
    if (filePath.empty())
    {
        return Fail(error, "Scene file path is empty.");
    }

    try
    {
        Json entities = Json::array();
        scene.ForEachEntity([&entities, componentRegistry](const Entity& entity)
        {
            entities.push_back(ToJson(entity, componentRegistry));
        });

        const Json document{
            {"schema_version", kSceneSchemaVersion},
            {"scene_name", scene.GetName()},
            {"entities", std::move(entities)}
        };

        const std::string serialized = document.dump(4) + "\n";
        if (!WriteFileAtomically(filePath, serialized, false, error))
            return false;
    }
    catch (const std::exception& exception)
    {
        return Fail(error, exception.what());
    }

    if (error != nullptr)
    {
        error->clear();
    }
    return true;
}

bool SceneSerializer::Load(
    Scene& scene,
    const std::filesystem::path& filePath,
    std::string* error,
    const ComponentRegistry* componentRegistry)
{
    try
    {
        std::ifstream input(filePath, std::ios::binary);
        if (!input)
        {
            return Fail(error, "Could not open scene file.");
        }

        const Json document = Json::parse(input);
        if (document.at("schema_version").get<std::uint32_t>() != kSceneSchemaVersion)
        {
            return Fail(error, "Unsupported scene schema version.");
        }

        const std::string sceneName = document.at("scene_name").get<std::string>();
        const Json& entities = document.at("entities");
        if (!entities.is_array())
        {
            return Fail(error, "Scene entities field must be an array.");
        }

        Scene loadedScene(sceneName);
        EntityId maxId = 0;

        for (const Json& value : entities)
        {
            if (!value.is_object())
            {
                return Fail(error, "Scene contains a non-object entity.");
            }

            EntityId id = 0;
            if (!ReadEntityId(value.at("id"), id) || loadedScene.m_entities.contains(id))
            {
                return Fail(error, "Scene contains an invalid or duplicate entity ID.");
            }

            const std::string name = value.at("name").get<std::string>();
            const bool active = value.at("active").get<bool>();
            const Json& transformJson = value.at("transform");
            const Json& renderJson = value.at("render");

            auto entity = std::unique_ptr<Entity>(new Entity(id, name));
            Transform2D& transform = entity->GetTransform();
            RenderComponent2D& render = entity->GetRenderComponent();

            if (!ReadVector2(transformJson.at("position"), transform.position) ||
                !ReadVector2(transformJson.at("scale"), transform.scale) ||
                !ReadFloat(transformJson.at("rotation"), transform.rotation) ||
                !ReadVector2(renderJson.at("size"), render.size) ||
                render.size.x < 0.0F || render.size.y < 0.0F)
            {
                return Fail(error, "Scene contains invalid transform or render dimensions.");
            }

            const Json& tint = renderJson.at("tint");
            if (!tint.is_array() || tint.size() != 4 ||
                !ReadColor(tint[0], render.red) ||
                !ReadColor(tint[1], render.green) ||
                !ReadColor(tint[2], render.blue) ||
                !ReadColor(tint[3], render.alpha))
            {
                return Fail(error, "Scene contains an invalid RGBA tint.");
            }

            if (value.contains("movement"))
            {
                const Json& movementJson = value.at("movement");
                entity->GetMovementComponent().enabled = movementJson.at("enabled").get<bool>();
                if (!ReadFloat(movementJson.at("speed"), entity->GetMovementComponent().speed) ||
                    entity->GetMovementComponent().speed < 0.0F)
                {
                    return Fail(error, "Scene contains an invalid movement speed.");
                }
            }

            render.visible = renderJson.at("visible").get<bool>();
            render.textureAssetId = renderJson.at("texture_asset_id").get<std::string>();
            // TextureHandle is deliberately not deserialized; it belongs to a live renderer.
            entity->SetActive(active);

            if (value.contains("components"))
            {
                const Json& components = value.at("components");
                if (!components.is_array())
                    return Fail(error, "Entity components field must be an array.");
                for (const Json& componentValue : components)
                {
                    if (!componentValue.is_object() || !componentValue.contains("type") ||
                        !componentValue.contains("data") || !componentValue.at("type").is_string() ||
                        componentRegistry == nullptr)
                        return Fail(error, "Scene contains a component that cannot be resolved.");

                    // Files written before component versions existed are version 1.
                    std::uint32_t componentVersion = 1;
                    if (componentValue.contains("version"))
                    {
                        const Json& versionValue = componentValue.at("version");
                        if (!versionValue.is_number_unsigned() && !versionValue.is_number_integer())
                            return Fail(error, "Scene contains an invalid component schema version.");
                        try
                        {
                            const auto version = versionValue.get<std::int64_t>();
                            if (version <= 0 ||
                                static_cast<std::uint64_t>(version) >
                                    std::numeric_limits<std::uint32_t>::max())
                                return Fail(error, "Scene contains an invalid component schema version.");
                            componentVersion = static_cast<std::uint32_t>(version);
                        }
                        catch (...)
                        {
                            return Fail(error, "Scene contains an invalid component schema version.");
                        }
                    }

                    auto component = componentRegistry->Create(
                        componentValue.at("type").get<std::string>(),
                        componentVersion, componentValue.at("data"));
                    if (!component)
                        return Fail(error, "Scene contains an unknown or invalid component type.");
                    entity->AddComponent(std::move(component));
                }
            }

            if (!MigrateLegacyColliders(*entity, error)) return false;

            loadedScene.m_entities.emplace(id, std::move(entity));
            if (id > maxId)
            {
                maxId = id;
            }
        }

        loadedScene.m_nextEntityId =
            maxId == std::numeric_limits<EntityId>::max() ? 0 : maxId + 1;

        scene.m_name = std::move(loadedScene.m_name);
        scene.m_nextEntityId = loadedScene.m_nextEntityId;
        scene.m_entities.swap(loadedScene.m_entities);
    }
    catch (const std::exception& exception)
    {
        return Fail(error, exception.what());
    }

    if (error != nullptr)
    {
        error->clear();
    }
    return true;
}
} // namespace ember
