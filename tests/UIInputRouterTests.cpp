#include "Engine/UI/UIInputRouter2D.hpp"

#include <iostream>
#include <memory>

namespace
{
class ClickCounterComponent final : public ember::Component
{
public:
    int clicks = 0;
    [[nodiscard]] std::unique_ptr<ember::Component> Clone() const override
    {
        return std::make_unique<ClickCounterComponent>(*this);
    }
    void OnUIButtonClicked(ember::Entity&) noexcept override { ++clicks; }
};

int failures = 0;

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << message << '\n';
        ++failures;
    }
    else
    {
        std::cout << "[PASS] " << message << '\n';
    }
}
}

int main()
{
    ember::Scene scene("UI Input Test");
    auto& canvasEntity = scene.CreateEntity("Canvas");
    auto& canvas = canvasEntity.AddComponent<ember::UICanvas2D>();
    canvas.referenceResolution = {1280.0F, 720.0F};

    auto& buttonEntity = scene.CreateEntity("Button");
    auto& button = buttonEntity.AddComponent<ember::UIButton2D>();
    button.position = {100.0F, 50.0F};
    button.size = {200.0F, 80.0F};

    ember::UIInputRouter2D router;
    const ember::Vector2 viewport{1280.0F, 800.0F};
    const ember::Vector2 inside{150.0F, 110.0F}; // Letterbox offset is 40 px.

    router.Update(scene, &canvas, viewport, inside, true, true, false);
    Check(router.GetState(buttonEntity.GetId()).hovered,
        "Pointer hover resolves through Canvas letterboxing");
    Check(router.GetState(buttonEntity.GetId()).pressed,
        "Press captures the hovered Button");
    Check(router.GetEvents().empty(), "Press does not emit a click");

    router.Update(scene, &canvas, viewport, {500.0F, 500.0F}, true, false, false);
    Check(!router.GetState(buttonEntity.GetId()).hovered &&
        router.GetState(buttonEntity.GetId()).pressed,
        "Captured Button remains pressed while pointer is dragged outside");

    router.Update(scene, &canvas, viewport, {500.0F, 500.0F}, false, false, true);
    Check(router.GetEvents().empty() && !router.GetState(buttonEntity.GetId()).pressed,
        "Release outside cancels the click");

    router.Update(scene, &canvas, viewport, inside, true, true, false);
    router.Update(scene, &canvas, viewport, inside, false, false, true);
    Check(router.GetEvents().size() == 1 &&
        router.GetEvents().front().entity == buttonEntity.GetId(),
        "Release over the captured Button emits one click event");
    Check(router.GetState(buttonEntity.GetId()).clicked,
        "Clicked state is exposed for the release frame");

    button.interactable = false;
    router.Update(scene, &canvas, viewport, inside, true, true, false);
    Check(!router.GetState(buttonEntity.GetId()).hovered &&
        router.GetEvents().empty(),
        "Non-interactable Button ignores pointer input");

    button.interactable = true;
    auto& overlappingEntity = scene.CreateEntity("Top Button");
    auto& overlappingButton = overlappingEntity.AddComponent<ember::UIButton2D>();
    overlappingButton.position = button.position;
    overlappingButton.size = button.size;
    router.Update(scene, &canvas, viewport, inside, false, false, false);
    Check(router.GetState(overlappingEntity.GetId()).hovered &&
        !router.GetState(buttonEntity.GetId()).hovered,
        "Topmost Button receives hover when Buttons overlap");

    router.Update(scene, &canvas, viewport, {150.0F, 20.0F}, false, false, false);
    Check(!router.GetState(overlappingEntity.GetId()).hovered,
        "Letterbox area does not hit-test UI Buttons");

    overlappingEntity.SetActive(false);
    router.Update(scene, &canvas, viewport, inside, false, false, false);
    Check(router.GetState(buttonEntity.GetId()).hovered,
        "Inactive topmost Entity is skipped during hit testing");

    router.Reset();
    Check(router.GetEvents().empty() && !router.GetState(buttonEntity.GetId()).hovered,
        "Reset clears transient interaction state and events");

    // Exercise the complete path from pointer input through the generated
    // event to the runtime Component callback.
    ember::Scene callbackScene("UI Click Callback Test");
    auto& callbackCanvasEntity = callbackScene.CreateEntity("Canvas");
    auto& callbackCanvas = callbackCanvasEntity.AddComponent<ember::UICanvas2D>();
    auto& callbackButtonEntity = callbackScene.CreateEntity("Callback Button");
    auto& callbackButton = callbackButtonEntity.AddComponent<ember::UIButton2D>();
    callbackButton.position = {20.0F, 30.0F};
    callbackButton.size = {100.0F, 50.0F};
    auto& clickCounter = callbackButtonEntity.AddComponent<ClickCounterComponent>();
    callbackScene.Start();

    ember::UIInputRouter2D callbackRouter;
    callbackRouter.Update(callbackScene, &callbackCanvas, {1280.0F, 720.0F},
        {40.0F, 40.0F}, true, true, false);
    callbackRouter.Update(callbackScene, &callbackCanvas, {1280.0F, 720.0F},
        {40.0F, 40.0F}, false, false, true);
    for (const auto& event : callbackRouter.GetEvents())
        (void)callbackScene.DispatchUIButtonClicked(event.entity);
    Check(clickCounter.clicks == 1,
        "Pointer release dispatches one runtime Component click callback");

    callbackRouter.Update(callbackScene, &callbackCanvas, {1280.0F, 720.0F},
        {40.0F, 40.0F}, true, true, false);
    callbackRouter.Update(callbackScene, &callbackCanvas, {1280.0F, 720.0F},
        {300.0F, 300.0F}, false, false, true);
    for (const auto& event : callbackRouter.GetEvents())
        (void)callbackScene.DispatchUIButtonClicked(event.entity);
    Check(clickCounter.clicks == 1,
        "Pointer release outside the Button does not invoke its callback");

    callbackButton.enabled = false;
    callbackRouter.Update(callbackScene, &callbackCanvas, {1280.0F, 720.0F},
        {40.0F, 40.0F}, true, true, false);
    callbackRouter.Update(callbackScene, &callbackCanvas, {1280.0F, 720.0F},
        {40.0F, 40.0F}, false, false, true);
    for (const auto& event : callbackRouter.GetEvents())
        (void)callbackScene.DispatchUIButtonClicked(event.entity);
    Check(clickCounter.clicks == 1,
        "Disabled Button cannot trigger a runtime Component callback");
    callbackScene.Stop();

    if (failures != 0)
    {
        std::cerr << failures << " UI input test(s) failed.\n";
        return 1;
    }
    std::cout << "All UI input router tests passed.\n";
    return 0;
}
