#include <iostream>
#include "pch.h"
#include "scene/Entity.h"
#include "scene/Scene.h"

using namespace Zephyr;

struct Position : Component<Position>
{
    float x = 0.f;
    float y = 0.f;
};

struct Velocity : Component<Velocity>
{
    float dx = 1.f;
    float dy = 1.f;
};

class MoveSystem : public System<MoveSystem>
{
public:
    MoveSystem(Engine* engine) : System(engine, {Velocity::ID, Position::ID}, {}, {}) {
    
    }
    ~MoveSystem() override = default;

    void Execute(float delta, const std::vector<Entity*>& entities) override
    {
        std::cout << "Total number of " << entities.size() << " entities get executed\n";
        for (auto& entity : entities)
        {
            Velocity* velocity = entity->GetComponent<Velocity>();
            Position* position = entity->GetComponent<Position>();

            position->x += velocity->dx * delta;
            position->y += velocity->dy * delta;
        }
    }
};

int main() { 

    assert(Position::ID != Velocity::ID);
    Scene myScene(nullptr, "TestScene");

    EntityHandle e1 = myScene.CreateEntity();
    EntityHandle e2 = myScene.CreateEntity();
    EntityHandle e3 = myScene.CreateEntity();

    myScene.AddComponent<Velocity>(e1);
    myScene.AddComponent<Position>(e1);
    myScene.AddComponent<Velocity>(e2);
    myScene.AddComponent<Position>(e2);
    myScene.AddComponent<Velocity>(e3);

    auto e1p = myScene.GetComponent<Position>(e1);

    auto e1v = myScene.GetComponent<Velocity>(e1);

    auto e2p = myScene.GetComponent<Position>(e2);
    e2p->x   = 20.f;
    e2p->y   = 5.f;

    auto e2v = myScene.GetComponent<Velocity>(e2);
    e2v->dx  = -3.f;
    e2v->dy  = 4.f;

    myScene.AddSystem<MoveSystem>();

    for (uint32_t i = 0; i < 100; i++)
    {
        myScene.Tick(1.);
    }

    assert(e2p->x == 20.f + 100 * -3.f);
    assert(e2p->y == 5.f + 100 * 4.f);
    assert(e1p->x == 0.f + 100 * 1.f);
    assert(e1p->y == 0.f + 100 * 1.f);
}