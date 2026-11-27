#include "Engine/UI/UIComponentRegistration.hpp"
#include "Engine/UI/UILayout2D.hpp"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
int failures = 0;
void Check(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "[FAIL] " << message << '\n';
        ++failures;
    } else {
        std::cout << "[PASS] " << message << '\n';
    }
}
bool Near(float a, float b) { return std::abs(a - b) < 0.001F; }
}

int main()
{
    ember::ComponentRegistry registry;
    Check(ember::RegisterUIComponents(registry), "UI Canvas and Panel register");
    Check(registry.Size() == 2, "UI registry contains the two initial UI component types");

    ember::UICanvas2D canvas;
    canvas.referenceResolution = {1920.0F, 1080.0F};
    nlohmann::json canvasData;
    Check(registry.Serialize(canvas, canvasData), "Canvas serializes");
    auto canvasCopy = registry.Create("ember.ui_canvas_2d", canvasData);
    const auto* restoredCanvas = dynamic_cast<const ember::UICanvas2D*>(canvasCopy.get());
    Check(restoredCanvas && Near(restoredCanvas->referenceResolution.x, 1920.0F) &&
        Near(restoredCanvas->referenceResolution.y, 1080.0F),
        "Canvas reference resolution round-trips");

    ember::UIPanel2D panel;
    panel.position = {100.0F, 80.0F};
    panel.size = {400.0F, 200.0F};
    panel.red = 0.8F;
    panel.green = 0.4F;
    nlohmann::json panelData;
    Check(registry.Serialize(panel, panelData), "Panel serializes");
    auto panelCopy = registry.Create("ember.ui_panel_2d", panelData);
    const auto* restoredPanel = dynamic_cast<const ember::UIPanel2D*>(panelCopy.get());
    Check(restoredPanel && Near(restoredPanel->position.x, 100.0F) &&
        Near(restoredPanel->size.y, 200.0F) && Near(restoredPanel->red, 0.8F) &&
        Near(restoredPanel->green, 0.4F),
        "Panel geometry and color round-trip");

    auto cloned = panel.Clone();
    const auto* clonedPanel = dynamic_cast<const ember::UIPanel2D*>(cloned.get());
    Check(clonedPanel && Near(clonedPanel->position.x, panel.position.x) &&
        Near(clonedPanel->alpha, panel.alpha), "Panel clones authoring data");

    ember::UICanvasLayout2D layout;
    Check(ember::ResolveUICanvasLayout(canvas, {1280.0F, 720.0F}, layout) &&
        Near(layout.scale, 2.0F / 3.0F) && Near(layout.origin.x, 0.0F) &&
        Near(layout.origin.y, 0.0F),
        "Canvas fits matching aspect ratio");
    Check(ember::ResolveUICanvasLayout(canvas, {1280.0F, 800.0F}, layout) &&
        Near(layout.scale, 2.0F / 3.0F) && Near(layout.origin.y, 40.0F),
        "Canvas letterboxes vertically while preserving aspect ratio");
    ember::UIRect2D rect;
    Check(ember::ResolveUIPanelRect(panel, layout, rect) &&
        Near(rect.position.x, 66.6667F) && Near(rect.position.y, 93.3333F) &&
        Near(rect.size.x, 266.6667F) && Near(rect.size.y, 133.3333F),
        "Panel rectangle resolves in viewport coordinates");

    panel.size.x = 0.0F;
    Check(!panel.IsConfigurationValid() &&
        !ember::ResolveUIPanelRect(panel, layout, rect),
        "Panel rejects zero-sized geometry");
    panel.size = {20.0F, 20.0F};
    panel.alpha = std::numeric_limits<float>::quiet_NaN();
    Check(!panel.IsConfigurationValid(), "Panel rejects non-finite color channels");
    canvas.referenceResolution.x = 0.0F;
    Check(!canvas.IsConfigurationValid() &&
        !ember::ResolveUICanvasLayout(canvas, {1280.0F, 720.0F}, layout),
        "Canvas rejects invalid reference resolution");
    canvas.referenceResolution = {1280.0F, 720.0F};
    Check(!ember::ResolveUICanvasLayout(canvas, {0.0F, 720.0F}, layout),
        "Canvas rejects invalid viewport dimensions");

    const auto* panelProperties = registry.FindProperties(panel);
    Check(panelProperties && panelProperties->size() == 7,
        "Panel exposes geometry, color and enabled Inspector properties");

    if (failures != 0) {
        std::cerr << failures << " UI test(s) failed.\n";
        return 1;
    }
    std::cout << "All UI component tests passed.\n";
    return 0;
}
