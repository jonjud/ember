#include "Engine/Scene/Scene.hpp"

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

class CounterComponent final : public ember::Component
{
public:
    int creates = 0;
    int updates = 0;
    int destroys = 0;
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
};
}

int main()
{
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

    Check(active.RemoveComponent<CounterComponent>(), "Entity removes a component before start");
    Check(!active.HasComponent<CounterComponent>(), "Removed component is no longer present");
    CounterComponent& counter = active.AddComponent<CounterComponent>();

    scene.Start();
    scene.Start();
    Check(scene.IsStarted(), "Scene enters the started state");
    Check(counter.creates == 1, "OnCreate runs once when the scene starts");
    Check(inactive.GetComponent<CounterComponent>()->creates == 1,
        "OnCreate runs for components on inactive entities");

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

    scene.Update(0.25F);
    scene.Update(-1.0F);
    Check(counter.updates == 2, "OnUpdate runs for active entities only");
    Check(std::fabs(counter.lastDelta) < 0.0001F,
        "Negative delta time is clamped to zero");
    Check(inactive.GetComponent<CounterComponent>()->updates == 0,
        "Inactive entities do not receive OnUpdate");

    scene.Stop();
    scene.Stop();
    Check(counter.destroys == 1, "OnDestroy runs once when the scene stops");
    Check(inactive.GetComponent<CounterComponent>()->destroys == 1,
        "OnDestroy runs for inactive entities too");

    ember::Scene clone = scene.Clone();
    const auto* clonedCounter = clone.FindEntity(active.GetId())->GetComponent<CounterComponent>();
    Check(clonedCounter != nullptr && clonedCounter != &counter,
        "Scene clone creates independent component instances");
    Check(clonedCounter != nullptr && clonedCounter->creates == 1 &&
          clonedCounter->updates == 2 && clonedCounter->destroys == 1,
        "Scene clone preserves component state");

    scene.Start();
    Check(counter.creates == 2, "A stopped scene can be started again");
    scene.Stop();
    Check(counter.destroys == 2, "OnDestroy runs when a restarted scene stops");
    const ember::EntityId activeId = active.GetId();
    Check(scene.DestroyEntity(activeId), "Stopped scene can destroy an entity");
    Check(scene.FindEntity(activeId) == nullptr, "Destroyed entity is removed from the scene");
    Check(clone.FindEntity(activeId)->GetComponent<CounterComponent>()->destroys == 1,
        "Cloned scene lifecycle remains independent");
    scene.Stop();

    if (g_failures != 0)
    {
        std::cerr << g_failures << " component test(s) failed.\n";
        return 1;
    }
    std::cout << "All component tests passed.\n";
    return 0;
}
