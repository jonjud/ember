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

    ok = registry.Register<UIText2D>("ember.ui_text_2d", "UI Text 2D",
        [](const Component& base) {
            const auto& v = static_cast<const UIText2D&>(base);
            return nlohmann::json{{"text",v.text},{"position",{v.position.x,v.position.y}},
                {"font_size",v.fontSize},{"red",v.red},{"green",v.green},
                {"blue",v.blue},{"alpha",v.alpha},{"enabled",v.enabled}};
        },
        [](Component& base, const nlohmann::json& d) {
            if (!d.is_object() || !d.contains("text") || !d.at("text").is_string() ||
                !d.contains("position") || !d.at("position").is_array() || d.at("position").size()!=2 ||
                !d.at("position")[0].is_number() || !d.at("position")[1].is_number() ||
                !d.contains("font_size") || !d.at("font_size").is_number() ||
                !d.contains("red") || !d.at("red").is_number() ||
                !d.contains("green") || !d.at("green").is_number() ||
                !d.contains("blue") || !d.at("blue").is_number() ||
                !d.contains("alpha") || !d.at("alpha").is_number() ||
                !d.contains("enabled") || !d.at("enabled").is_boolean()) return false;
            try {
                auto& v=static_cast<UIText2D&>(base);
                v.text=d.at("text").get<std::string>();
                v.position={d.at("position")[0].get<float>(),d.at("position")[1].get<float>()};
                v.fontSize=d.at("font_size").get<float>();
                v.red=d.at("red").get<float>(); v.green=d.at("green").get<float>();
                v.blue=d.at("blue").get<float>(); v.alpha=d.at("alpha").get<float>();
                v.enabled=d.at("enabled").get<bool>();
                return v.IsConfigurationValid();
            } catch (...) { return false; }
        });
    ok = registry.RegisterProperty<UIText2D>("ember.ui_text_2d","text","Text",&UIText2D::text) && ok;
    ok = registry.RegisterVector2Property<UIText2D>("ember.ui_text_2d","position","Position",&UIText2D::position) && ok;
    ok = registry.RegisterProperty<UIText2D>("ember.ui_text_2d","font_size","Font Size",&UIText2D::fontSize) && ok;
    ok = registry.RegisterProperty<UIText2D>("ember.ui_text_2d","red","Red",&UIText2D::red) && ok;
    ok = registry.RegisterProperty<UIText2D>("ember.ui_text_2d","green","Green",&UIText2D::green) && ok;
    ok = registry.RegisterProperty<UIText2D>("ember.ui_text_2d","blue","Blue",&UIText2D::blue) && ok;
    ok = registry.RegisterProperty<UIText2D>("ember.ui_text_2d","alpha","Alpha",&UIText2D::alpha) && ok;
    ok = registry.RegisterProperty<UIText2D>("ember.ui_text_2d","enabled","Enabled",&UIText2D::enabled) && ok;

    ok = registry.Register<UIButton2D>("ember.ui_button_2d", "UI Button 2D",
        [](const Component& base) {
            const auto& v = static_cast<const UIButton2D&>(base);
            return nlohmann::json{{"text",v.text},{"position",{v.position.x,v.position.y}},
                {"size",{v.size.x,v.size.y}},{"red",v.red},{"green",v.green},
                {"blue",v.blue},{"alpha",v.alpha},{"enabled",v.enabled},
                {"interactable",v.interactable}};
        },
        [](Component& base, const nlohmann::json& d) {
            if (!d.is_object() || !d.contains("text") || !d.at("text").is_string() ||
                !d.contains("position") || !d.at("position").is_array() || d.at("position").size()!=2 ||
                !d.at("position")[0].is_number() || !d.at("position")[1].is_number() ||
                !d.contains("size") || !d.at("size").is_array() || d.at("size").size()!=2 ||
                !d.at("size")[0].is_number() || !d.at("size")[1].is_number() ||
                !d.contains("red") || !d.at("red").is_number() ||
                !d.contains("green") || !d.at("green").is_number() ||
                !d.contains("blue") || !d.at("blue").is_number() ||
                !d.contains("alpha") || !d.at("alpha").is_number() ||
                !d.contains("enabled") || !d.at("enabled").is_boolean() ||
                !d.contains("interactable") || !d.at("interactable").is_boolean()) return false;
            try {
                auto& v=static_cast<UIButton2D&>(base);
                v.text=d.at("text").get<std::string>();
                v.position={d.at("position")[0].get<float>(),d.at("position")[1].get<float>()};
                v.size={d.at("size")[0].get<float>(),d.at("size")[1].get<float>()};
                v.red=d.at("red").get<float>(); v.green=d.at("green").get<float>();
                v.blue=d.at("blue").get<float>(); v.alpha=d.at("alpha").get<float>();
                v.enabled=d.at("enabled").get<bool>();
                v.interactable=d.at("interactable").get<bool>();
                return v.IsConfigurationValid();
            } catch (...) { return false; }
        });
    ok = registry.RegisterProperty<UIButton2D>("ember.ui_button_2d","text","Text",&UIButton2D::text) && ok;
    ok = registry.RegisterVector2Property<UIButton2D>("ember.ui_button_2d","position","Position",&UIButton2D::position) && ok;
    ok = registry.RegisterVector2Property<UIButton2D>("ember.ui_button_2d","size","Size",&UIButton2D::size) && ok;
    ok = registry.RegisterProperty<UIButton2D>("ember.ui_button_2d","red","Red",&UIButton2D::red) && ok;
    ok = registry.RegisterProperty<UIButton2D>("ember.ui_button_2d","green","Green",&UIButton2D::green) && ok;
    ok = registry.RegisterProperty<UIButton2D>("ember.ui_button_2d","blue","Blue",&UIButton2D::blue) && ok;
    ok = registry.RegisterProperty<UIButton2D>("ember.ui_button_2d","alpha","Alpha",&UIButton2D::alpha) && ok;
    ok = registry.RegisterProperty<UIButton2D>("ember.ui_button_2d","enabled","Enabled",&UIButton2D::enabled) && ok;
    ok = registry.RegisterProperty<UIButton2D>("ember.ui_button_2d","interactable","Interactable",&UIButton2D::interactable) && ok;
    return ok;
}
} // namespace ember
