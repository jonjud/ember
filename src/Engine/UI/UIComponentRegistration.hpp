#pragma once

#include "Engine/Scene/ComponentRegistry.hpp"
#include "Engine/UI/UIComponents2D.hpp"

namespace ember
{
inline bool RegisterUIComponents(ComponentRegistry& registry)
{
    bool ok = registry.Register<UICanvas2D>("ember.ui_canvas_2d", "UI Canvas 2D",
        [](const Component& base) {
            const auto& canvas = static_cast<const UICanvas2D&>(base);
            return nlohmann::json{
                {"reference_resolution", {canvas.referenceResolution.x, canvas.referenceResolution.y}},
                {"enabled", canvas.enabled}};
        },
        [](Component& base, const nlohmann::json& data) {
            if (!data.is_object() || !data.contains("reference_resolution") ||
                !data.at("reference_resolution").is_array() ||
                data.at("reference_resolution").size() != 2 ||
                !data.at("reference_resolution")[0].is_number() ||
                !data.at("reference_resolution")[1].is_number() ||
                !data.contains("enabled") || !data.at("enabled").is_boolean())
                return false;
            try {
                auto& canvas = static_cast<UICanvas2D&>(base);
                canvas.referenceResolution = {
                    data.at("reference_resolution")[0].get<float>(),
                    data.at("reference_resolution")[1].get<float>()};
                canvas.enabled = data.at("enabled").get<bool>();
                return canvas.IsConfigurationValid();
            } catch (...) { return false; }
        });
    ok = registry.RegisterVector2Property<UICanvas2D>(
        "ember.ui_canvas_2d", "reference_resolution", "Reference Resolution",
        &UICanvas2D::referenceResolution) && ok;
    ok = registry.RegisterProperty<UICanvas2D>(
        "ember.ui_canvas_2d", "enabled", "Enabled", &UICanvas2D::enabled) && ok;

    ok = registry.Register<UIPanel2D>("ember.ui_panel_2d", "UI Panel 2D",
        [](const Component& base) {
            const auto& panel = static_cast<const UIPanel2D&>(base);
            return nlohmann::json{
                {"position", {panel.position.x, panel.position.y}},
                {"size", {panel.size.x, panel.size.y}},
                {"red", panel.red}, {"green", panel.green},
                {"blue", panel.blue}, {"alpha", panel.alpha},
                {"enabled", panel.enabled}};
        },
        [](Component& base, const nlohmann::json& data) {
            if (!data.is_object() ||
                !data.contains("position") || !data.at("position").is_array() ||
                data.at("position").size() != 2 ||
                !data.at("position")[0].is_number() || !data.at("position")[1].is_number() ||
                !data.contains("size") || !data.at("size").is_array() ||
                data.at("size").size() != 2 ||
                !data.at("size")[0].is_number() || !data.at("size")[1].is_number() ||
                !data.contains("red") || !data.at("red").is_number() ||
                !data.contains("green") || !data.at("green").is_number() ||
                !data.contains("blue") || !data.at("blue").is_number() ||
                !data.contains("alpha") || !data.at("alpha").is_number() ||
                !data.contains("enabled") || !data.at("enabled").is_boolean())
                return false;
            try {
                auto& panel = static_cast<UIPanel2D&>(base);
                panel.position = {data.at("position")[0].get<float>(),
                    data.at("position")[1].get<float>()};
                panel.size = {data.at("size")[0].get<float>(),
                    data.at("size")[1].get<float>()};
                panel.red = data.at("red").get<float>();
                panel.green = data.at("green").get<float>();
                panel.blue = data.at("blue").get<float>();
                panel.alpha = data.at("alpha").get<float>();
                panel.enabled = data.at("enabled").get<bool>();
                return panel.IsConfigurationValid();
            } catch (...) { return false; }
        });
    ok = registry.RegisterVector2Property<UIPanel2D>(
        "ember.ui_panel_2d", "position", "Position", &UIPanel2D::position) && ok;
    ok = registry.RegisterVector2Property<UIPanel2D>(
        "ember.ui_panel_2d", "size", "Size", &UIPanel2D::size) && ok;
    ok = registry.RegisterProperty<UIPanel2D>(
        "ember.ui_panel_2d", "red", "Red", &UIPanel2D::red) && ok;
    ok = registry.RegisterProperty<UIPanel2D>(
        "ember.ui_panel_2d", "green", "Green", &UIPanel2D::green) && ok;
    ok = registry.RegisterProperty<UIPanel2D>(
        "ember.ui_panel_2d", "blue", "Blue", &UIPanel2D::blue) && ok;
    ok = registry.RegisterProperty<UIPanel2D>(
        "ember.ui_panel_2d", "alpha", "Alpha", &UIPanel2D::alpha) && ok;
    ok = registry.RegisterProperty<UIPanel2D>(
        "ember.ui_panel_2d", "enabled", "Enabled", &UIPanel2D::enabled) && ok;
    return ok;
}
} // namespace ember
