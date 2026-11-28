#include "Engine/Scene/ComponentRegistry.hpp"
#include "Engine/Scene/AudioComponentRegistration.hpp"
#include "Editor/MultiEdit.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace
{
int failures = 0;
void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << message << '\n';
        ++failures;
    }
    else std::cout << "[PASS] " << message << '\n';
}

class MarkerComponent final : public ember::Component
{
public:
    int value = 42;
    bool enabled = true;
    float speed = 1.5F;
    std::string label = "initial";
    [[nodiscard]] std::unique_ptr<ember::Component> Clone() const override
    {
        return std::make_unique<MarkerComponent>(*this);
    }
};

class OtherComponent final : public ember::Component
{
public:
    int otherValue = 0;
    [[nodiscard]] std::unique_ptr<ember::Component> Clone() const override
    {
        return std::make_unique<OtherComponent>(*this);
    }
};

class MigratingComponent final : public ember::Component
{
public:
    int amount = 0;
    [[nodiscard]] std::unique_ptr<ember::Component> Clone() const override
    {
        return std::make_unique<MigratingComponent>(*this);
    }
};
}

int main()
{
    ember::ComponentRegistry registry;
    Check(registry.IsEmpty(), "New registry starts empty");
    Check(!registry.Register<MarkerComponent>("", "Marker"), "Empty stable IDs are rejected");
    Check(!registry.Register<MarkerComponent>("marker", ""), "Empty display names are rejected");
    Check(registry.Register<MarkerComponent>("example.marker", "Marker"),
        "Component type can be registered");
    Check(!registry.Register<MarkerComponent>("example.marker.copy", "Duplicate Type"),
        "A C++ component type cannot be registered twice");
    Check(!registry.Register<OtherComponent>("example.marker", "Duplicate ID"),
        "Stable component IDs must be unique");
    Check(registry.Register<OtherComponent>("example.other", "Other"),
        "A second component type can be registered");

    const auto* markerInfo = registry.Find("example.marker");
    Check(markerInfo != nullptr && markerInfo->displayName == "Marker",
        "Registered metadata can be found by stable ID");

    MarkerComponent source;
    const auto* sourceInfo = registry.Find(source);
    Check(sourceInfo != nullptr && sourceInfo->id == "example.marker",
        "Registered metadata can be found by component instance");

    auto created = registry.Create("example.marker");
    auto* marker = dynamic_cast<MarkerComponent*>(created.get());
    Check(marker != nullptr && marker->value == 42,
        "Factory creates the registered component type");
    Check(registry.Create("missing") == nullptr,
        "Unknown component IDs return no instance");

    const auto types = registry.GetRegisteredTypes();
    Check(types.size() == 2 && types[0].id == "example.marker" &&
          types[1].id == "example.other",
          "Registered types are listed in deterministic ID order");
    Check(registry.Size() == 2, "Registry reports its type count");

    Check(registry.RegisterProperty<MarkerComponent>(
        "example.marker", "value", "Value", &MarkerComponent::value),
        "Integer component property can be registered");
    Check(registry.RegisterProperty<MarkerComponent>(
        "example.marker", "enabled", "Enabled", &MarkerComponent::enabled),
        "Boolean component property can be registered");
    Check(registry.RegisterProperty<MarkerComponent>(
        "example.marker", "speed", "Speed", &MarkerComponent::speed),
        "Float component property can be registered");
    Check(registry.RegisterProperty<MarkerComponent>(
        "example.marker", "label", "Label", &MarkerComponent::label),
        "String component property can be registered");
    Check(!registry.RegisterProperty<MarkerComponent>(
        "example.marker", "value", "Duplicate", &MarkerComponent::value),
        "Duplicate property IDs are rejected");
    Check(!registry.RegisterProperty<OtherComponent>(
        "example.marker", "wrong-owner", "Wrong Owner", &OtherComponent::otherValue),
        "Properties cannot be registered against a different component type");

    Check(ember::editor::HasMixedValues({1, 1, 1}) == false,
        "Multi-edit detects uniform property values");
    Check(ember::editor::HasMixedValues({1, 2}) == true,
        "Multi-edit detects mixed property values");
    MarkerComponent first;
    MarkerComponent second;
    first.value = 10;
    second.value = 20;
    const auto* editableProperties = registry.FindProperties(first);
    const ember::ComponentPropertyInfo* editableValue = nullptr;
    if (editableProperties != nullptr)
        for (const auto& property : *editableProperties)
            if (property.id == "value") editableValue = &property;
    std::vector<ember::Component*> multiComponents{&first, &second};
    Check(editableValue != nullptr &&
          ember::editor::ApplyComponentPropertyAtomically(multiComponents, *editableValue, 77) &&
          first.value == 77 && second.value == 77,
          "Multi-edit applies a shared property value to every selected component");
    Check(editableValue != nullptr &&
          !ember::editor::ApplyComponentPropertyAtomically(multiComponents, *editableValue, "invalid") &&
          first.value == 77 && second.value == 77,
          "Multi-edit rejects invalid values without partially changing components");

    const auto* markerProperties = registry.FindProperties(source);
    Check(markerProperties != nullptr && markerProperties->size() == 4,
        "Property metadata can be queried by component instance");
    if (markerProperties != nullptr)
    {
        const auto findProperty = [markerProperties](const std::string& id)
            -> const ember::ComponentPropertyInfo*
        {
            for (const auto& property : *markerProperties)
                if (property.id == id) return &property;
            return nullptr;
        };
        const auto* integer = findProperty("value");
        const auto* boolean = findProperty("enabled");
        const auto* floating = findProperty("speed");
        const auto* string = findProperty("label");
        Check(integer && integer->type == ember::ComponentPropertyType::Integer &&
            integer->get(source).get<int>() == 42,
            "Integer descriptor reads component values");
        Check(integer && integer->set(source, 73) && source.value == 73,
            "Integer descriptor updates component values");
        Check(integer && !integer->set(source, "not an integer") && source.value == 73,
            "Integer descriptor rejects values with the wrong type");
        Check(integer && !integer->set(source, 999999999999LL) && source.value == 73,
            "Integer descriptor rejects values outside the supported range");
        Check(boolean && boolean->type == ember::ComponentPropertyType::Boolean &&
            boolean->set(source, false) && !source.enabled,
            "Boolean descriptor reads and updates component values");
        Check(floating && floating->type == ember::ComponentPropertyType::Float &&
            floating->set(source, 2.75) && source.speed == 2.75F,
            "Float descriptor reads and updates component values");
        Check(string && string->type == ember::ComponentPropertyType::String &&
            string->set(source, "updated") && source.label == "updated",
            "String descriptor reads and updates component values");
        Check(string && !string->set(source, std::string(4097, 'x')) &&
            source.label == "updated",
            "String descriptor rejects values exceeding the editor buffer limit");
    }

    Check(registry.Register<MigratingComponent>("example.migrating", "Migrating",
        [](const ember::Component& component) -> nlohmann::json {
            return nlohmann::json{{"amount",
                static_cast<const MigratingComponent&>(component).amount}};
        },
        [](ember::Component& component, const nlohmann::json& data) {
            if (!data.is_object() || !data.contains("amount") ||
                !data.at("amount").is_number_integer()) return false;
            static_cast<MigratingComponent&>(component).amount =
                data.at("amount").get<int>();
            return true;
        }, 2), "Versioned component type can be registered");
    Check(!registry.Register<MigratingComponent>("example.bad-version", "Bad", {}, {}, 0),
        "Component schema version zero is rejected");
    Check(registry.RegisterMigration("example.migrating", 1,
        [](nlohmann::json& data) {
            if (!data.is_object() || !data.contains("value") ||
                !data.at("value").is_number_integer()) return false;
            data["amount"] = data["value"];
            data.erase("value");
            return true;
        }), "A migration from schema version 1 can be registered");
    Check(!registry.RegisterMigration("example.migrating", 1,
        [](nlohmann::json&) { return true; }),
        "Duplicate component migrations are rejected");
    Check(!registry.RegisterMigration("example.migrating", 2,
        [](nlohmann::json&) { return true; }),
        "Migration from the current schema version is rejected");
    Check(!registry.RegisterMigration("example.migrating", 0,
        [](nlohmann::json&) { return true; }),
        "Migration from schema version zero is rejected");

    auto migrated = registry.Create("example.migrating", 1,
        nlohmann::json{{"value", 88}});
    const auto* migratedComponent =
        dynamic_cast<const MigratingComponent*>(migrated.get());
    Check(migratedComponent != nullptr && migratedComponent->amount == 88,
        "Older component data is migrated before deserialization");
    auto currentVersion = registry.Create("example.migrating", 2,
        nlohmann::json{{"amount", 42}});
    const auto* currentComponent =
        dynamic_cast<const MigratingComponent*>(currentVersion.get());
    Check(currentComponent != nullptr && currentComponent->amount == 42,
        "Current component schema data loads without migration");
    Check(registry.Create("example.migrating", 3,
        nlohmann::json{{"amount", 42}}) == nullptr,
        "Future component schema versions are rejected");


    ember::ComponentRegistry audioRegistry;
    Check(ember::RegisterAudioComponents(audioRegistry),
        "AudioSource2D registers with serialization and Inspector properties");
    ember::AudioSource2D audioSource;
    audioSource.audioAssetId = "audio/ambience";
    audioSource.volume = 0.65F;
    audioSource.loop = true;
    audioSource.playOnStart = true;
    audioSource.streaming = true;
    nlohmann::json audioData;
    Check(audioRegistry.Serialize(audioSource, audioData),
        "AudioSource2D configuration serializes");
    auto restoredAudio = audioRegistry.Create("ember.audio_source_2d", audioData);
    const auto* restoredSource = dynamic_cast<const ember::AudioSource2D*>(restoredAudio.get());
    Check(restoredSource && restoredSource->audioAssetId == "audio/ambience" &&
        restoredSource->volume == 0.65F && restoredSource->loop &&
        restoredSource->playOnStart && restoredSource->streaming,
        "AudioSource2D configuration round-trips through the component registry");
    const auto* audioProperties = audioRegistry.FindProperties(audioSource);
    Check(audioProperties && audioProperties->size() == 6,
        "AudioSource2D exposes all authoring properties to the Inspector");
    if (audioProperties) {
        const auto findAudioProperty = [audioProperties](const std::string& id)
            -> const ember::ComponentPropertyInfo* {
            for (const auto& property : *audioProperties)
                if (property.id == id) return &property;
            return nullptr;
        };
        const auto* volume = findAudioProperty("volume");
        const auto* asset = findAudioProperty("audio_asset_id");
        const auto* streaming = findAudioProperty("streaming");
        Check(volume && volume->set(audioSource, 1.25) && audioSource.volume == 1.25F,
            "AudioSource2D volume can be edited through the Inspector descriptor");
        Check(asset && asset->set(audioSource, "audio/new") &&
            audioSource.audioAssetId == "audio/new",
            "AudioSource2D asset reference can be edited through the Inspector descriptor");
        Check(streaming && streaming->set(audioSource, false) && !audioSource.streaming,
            "AudioSource2D streaming mode can be edited through the Inspector descriptor");
    }
    audioSource.volume = 5.0F;
    Check(!audioSource.IsConfigurationValid(),
        "AudioSource2D rejects volume outside the supported range");
    audioSource.volume = 1.0F;
    audioSource.audioAssetId.assign(257, 'x');
    Check(!audioSource.IsConfigurationValid(),
        "AudioSource2D rejects oversized asset IDs");

    if (failures != 0)
    {
        std::cerr << failures << " component registry test(s) failed.\n";
        return 1;
    }
    std::cout << "All component registry tests passed.\n";
    return 0;
}
