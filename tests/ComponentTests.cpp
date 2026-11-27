#include "Engine/Scene/Scene.hpp"
#include "Engine/UI/UIComponents2D.hpp"
#include "Engine/Platform/InputMap.hpp"

#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace
{
int g_failures = 0;
void Check(bool condition, const char* description)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << description << '\n';
        ++g_failures;
    }
    else std::cout << "[PASS] " << description << '\n';
}

class InputAwareComponent final : public ember::Component
{
public:
    bool down = false, pressed = false, released = false;
    [[nodiscard]] std::unique_ptr<ember::Component> Clone() const override
    {
        return std::make_unique<InputAwareComponent>(*this);
    }
    void OnUpdate(ember::Entity&, float, const ember::InputActionState& input) noexcept override
    {
        down = input.IsDown("Jump");
        pressed = input.WasPressed("Jump");
        released = input.WasReleased("Jump");
    }
};

class CounterComponent final : public ember::Component
{
public:
    int creates = 0;
    int updates = 0;
    int destroys = 0;
    int uiClicks = 0;
    float lastDelta = 0.0F;

    [[nodiscard]] std::unique_ptr<ember::Component> Clone() const override
    {
        return std::make_unique<CounterComponent>(*this);
    }
    void OnCreate(ember::Entity&) noexcept override { ++creates; }
    void OnUpdate(ember::Entity&, float deltaSeconds) noexcept override
    {
        ++updates;
        lastDelta = deltaSeconds;
    }
    void OnDestroy(ember::Entity&) noexcept override { ++destroys; }
    void OnUIButtonClicked(ember::Entity&) noexcept override { ++uiClicks; }
};
}

int main()
{
    std::cerr << "[TRACE] main entered\n" << std::flush;
    ember::Scene scene("Lifecycle Test");
    ember::Entity& active = scene.CreateEntity("Active");
    Check(!active.HasComponent<CounterComponent>(), "New entity has no counter component");
    Check(!active.RemoveComponent<CounterComponent>(), "Removing a missing component returns false");
    CounterComponent& activeCounter = active.AddComponent<CounterComponent>();
    Check(active.HasComponent<CounterComponent>(), "Entity reports an attached component");
    Check(active.GetComponent<CounterComponent>() == &activeCounter,
        "Entity retrieves a component by type");

    ember::Entity& inactive = scene.CreateEntity("Inactive");
    inactive.SetActive(false);
    inactive.AddComponent<CounterComponent>();

    ember::Entity& plainEntity = scene.CreateEntity("No Button");
    CounterComponent& plainCounter = plainEntity.AddComponent<CounterComponent>();

    Check(active.RemoveComponent<CounterComponent>(), "Entity removes a component before start");
    Check(!active.HasComponent<CounterComponent>(), "Removed component is no longer present");
    CounterComponent& counter = active.AddComponent<CounterComponent>();
    auto& activeButton = active.AddComponent<ember::UIButton2D>();

    Check(!scene.DispatchUIButtonClicked(active.GetId()),
        "UI click events are rejected before the Scene starts");
    std::cerr << "[TRACE] before lifecycle Scene::Start\n" << std::flush;
    scene.Start();
    std::cerr << "[TRACE] after first lifecycle Scene::Start\n" << std::flush;
    scene.Start();
    Check(scene.IsStarted(), "Scene enters the started state");
    Check(counter.creates == 1, "OnCreate runs once when the scene starts");
    Check(inactive.GetComponent<CounterComponent>()->creates == 1,
        "OnCreate runs for components on inactive entities");
    Check(scene.DispatchUIButtonClicked(active.GetId()) && counter.uiClicks == 1,
        "Scene dispatches a UI click to components on an active Button Entity");
    Check(!scene.DispatchUIButtonClicked(plainEntity.GetId()) && plainCounter.uiClicks == 0,
        "Scene rejects UI click dispatch to an active Entity without a Button");
    activeButton.interactable = false;
    Check(!scene.DispatchUIButtonClicked(active.GetId()) && counter.uiClicks == 1,
        "Scene rejects UI clicks for non-interactable Buttons");
    activeButton.interactable = true;
    activeButton.enabled = false;
    Check(!scene.DispatchUIButtonClicked(active.GetId()) && counter.uiClicks == 1,
        "Scene rejects UI clicks for disabled Buttons");
    activeButton.enabled = true;
    Check(!scene.DispatchUIButtonClicked(inactive.GetId()) &&
        inactive.GetComponent<CounterComponent>()->uiClicks == 0,
        "UI clicks are not dispatched to inactive Entities");
    Check(!scene.DispatchUIButtonClicked(999999),
        "UI click dispatch rejects an unknown Entity ID");

    bool addRejected = false;
    bool removeRejected = false;
    bool destroyRejected = false;
    try { active.AddComponent<CounterComponent>(); }
    catch (const std::logic_error&) { addRejected = true; }
    try { (void)active.RemoveComponent<CounterComponent>(); }
    catch (const std::logic_error&) { removeRejected = true; }
    try { (void)scene.DestroyEntity(active.GetId()); }
    catch (const std::logic_error&) { destroyRejected = true; }
    Check(addRejected && removeRejected && destroyRejected,
        "Scene and component structure is locked while running");

    std::cerr << "[TRACE] before lifecycle Scene::Update\n" << std::flush;
    scene.Update(0.25F);
    std::cerr << "[TRACE] after first lifecycle Scene::Update\n" << std::flush;
    scene.Update(-1.0F);
    Check(counter.updates == 2, "OnUpdate runs for active entities only");
    Check(std::fabs(counter.lastDelta) < 0.0001F,
        "Negative delta time is clamped to zero");
    Check(inactive.GetComponent<CounterComponent>()->updates == 0,
        "Inactive entities do not receive OnUpdate");

    std::cerr << "[TRACE] before lifecycle Scene::Stop\n" << std::flush;
    scene.Stop();
    std::cerr << "[TRACE] after first lifecycle Scene::Stop\n" << std::flush;
    scene.Stop();
    Check(!scene.DispatchUIButtonClicked(active.GetId()),
        "UI click events are rejected after the Scene stops");
    Check(counter.destroys == 1, "OnDestroy runs once when the scene stops");
    Check(inactive.GetComponent<CounterComponent>()->destroys == 1,
        "OnDestroy runs for inactive entities too");

    std::cerr << "[TRACE] before Scene::Clone\n" << std::flush;
    ember::Scene clone = scene.Clone();
    std::cerr << "[TRACE] after Scene::Clone\n" << std::flush;
    const auto* clonedCounter = clone.FindEntity(active.GetId())->GetComponent<CounterComponent>();
    Check(clonedCounter != nullptr && clonedCounter != &counter,
        "Scene clone creates independent component instances");
    Check(clonedCounter != nullptr && clonedCounter->creates == 1 &&
          clonedCounter->updates == 2 && clonedCounter->destroys == 1 &&
          clonedCounter->uiClicks == 1,
        "Scene clone preserves component state");

    std::cerr << "[TRACE] before restarted Scene::Start\n" << std::flush;
    scene.Start();
    std::cerr << "[TRACE] after restarted Scene::Start\n" << std::flush;
    Check(counter.creates == 2, "A stopped scene can be started again");
    scene.Stop();
    Check(counter.destroys == 2, "OnDestroy runs when a restarted scene stops");
    const ember::EntityId activeId = active.GetId();
    Check(scene.DestroyEntity(activeId), "Stopped scene can destroy an entity");
    Check(scene.FindEntity(activeId) == nullptr, "Destroyed entity is removed from the scene");
    Check(clone.FindEntity(activeId)->GetComponent<CounterComponent>()->destroys == 1,
        "Cloned scene lifecycle remains independent");
    scene.Stop();

    ember::Scene inputScene("Input Action Runtime");
    auto& inputEntity = inputScene.CreateEntity("Input Consumer");
    auto& inputConsumer = inputEntity.AddComponent<InputAwareComponent>();
    std::cerr << "[TRACE] before inputScene.Start\\n" << std::flush;
    inputScene.Start();
    std::cerr << "[TRACE] after inputScene.Start\\n" << std::flush;
    ember::InputMap inputMap;
    std::cerr << "[TRACE] before BindAction\\n" << std::flush;
    (void)inputMap.BindAction("Jump", SDL_SCANCODE_SPACE);
    std::cerr << "[TRACE] after BindAction\\n" << std::flush;
    std::cerr << "[TRACE] before first InputMap Update\\n" << std::flush;
    inputMap.Update({SDL_SCANCODE_SPACE});
    std::cerr << "[TRACE] after first InputMap Update\\n" << std::flush;
    std::cerr << "[TRACE] before Scene Update pressed\\n" << std::flush;
    inputScene.Update(0.016F, inputMap.GetActionState());
    std::cerr << "[TRACE] after Scene Update pressed\\n" << std::flush;
    Check(inputConsumer.down && inputConsumer.pressed && !inputConsumer.released,
        "Scene forwards a newly pressed action to gameplay Components");
    inputMap.Update({SDL_SCANCODE_SPACE});
    inputScene.Update(0.016F, inputMap.GetActionState());
    Check(inputConsumer.down && !inputConsumer.pressed && !inputConsumer.released,
        "Scene forwards held action state without repeating the pressed edge");
    std::cerr << "[TRACE] before empty InputMap Update\\n" << std::flush;
    inputMap.Update({});
    std::cerr << "[TRACE] after empty InputMap Update\\n" << std::flush;
    inputScene.Update(0.016F, inputMap.GetActionState());
    Check(!inputConsumer.down && !inputConsumer.pressed && inputConsumer.released,
        "Scene forwards released action edges to gameplay Components");
    std::cerr << "[TRACE] before inputScene.Stop\\n" << std::flush;
    inputScene.Stop();
    std::cerr << "[TRACE] after inputScene.Stop\\n" << std::flush;

    if (g_failures != 0)
    {
        std::cerr << g_failures << " component test(s) failed.\n";
        return 1;
    }
    std::cout << "All component tests passed.\n";
    return 0;
}
